#include "music.h"

#include <Tempest/Log>
#include <fstream>
#include <algorithm>

#include "dlscollection.h"
#include "directmusic.h"
#include "style.h"

using namespace Dx8;
using namespace Tempest;

// from DX8 SDK docs
static uint32_t musicOffset(uint32_t mtGridStart, int16_t nTimeOffset, const DMUS_IO_TIMESIG& timeSig) {
  const uint32_t ppq  = 768;
  const uint32_t mult = (ppq*4)/timeSig.bBeat;
  return uint32_t(nTimeOffset) +
         (mtGridStart / timeSig.wGridsPerBeat) * (mult) +
         (mtGridStart % timeSig.wGridsPerBeat) * (mult/timeSig.wGridsPerBeat);
  }

static bool offsetFromScale(const uint8_t degree, const uint32_t scale, uint8_t& offset) {
  uint8_t cnt=0;
  for(uint8_t i=0; i<24; i++) {
    if(scale & (1 << i)) {
      offset = i;
      if(cnt==degree)
        return true;
      cnt++;
      }
    }
  return false;
  }

static bool musicValueToMIDI(const DMUS_IO_STYLENOTE&             note,
                             const uint32_t&                      chord,
                             const std::vector<DMUS_IO_SUBCHORD>& subchords,
                             uint8_t&                             value) {
  if(note.bPlayModeFlags == DMUS_PLAYMODE_FIXED) {
    value = uint8_t(note.wMusicValue);
    return true;
    }

  uint32_t dwChordPattern = 0x00000091;
  uint32_t dwScalePattern = 0x00AB5AB5;

  if(subchords.size()>0) {
    dwChordPattern = subchords[0].dwChordPattern;
    dwScalePattern = subchords[0].dwScalePattern;
    }

  uint8_t octave    = ((note.wMusicValue & 0xF000) >> 12);
  uint8_t chordTone = ((note.wMusicValue & 0x0F00) >> 8);
  uint8_t scaleTone = ((note.wMusicValue & 0x00F0) >> 4);

  int accidentals = int8_t(note.wMusicValue & 0x000F);
  if(accidentals>7)
    accidentals = (accidentals - 16);

  int noteValue = ((chord & 0xFF000000) >> 24) + 12 * octave;
  uint8_t chordOffset = 0;
  uint8_t scaleOffset = 0;
  if(offsetFromScale(chordTone, dwChordPattern, chordOffset)) {
    noteValue += chordOffset;
    } else {
    noteValue += chordOffset+4; //FIXME
    //return false;
    }

  if(scaleTone && offsetFromScale(scaleTone, dwScalePattern >> chordOffset, scaleOffset)) {
    noteValue += scaleOffset;
    }

  noteValue += accidentals;
  while(noteValue<0)
    noteValue += 12;
  while(noteValue>127)
    noteValue -= 12;

  value = uint8_t(noteValue);
  return true;
  }

Music::Music(const Segment &s, DirectMusic &owner)
  :owner(&owner), intern(std::make_shared<Internal>()) {
  for(const auto& track : s.track) {
    auto& head = track.head;
    if(head.ckid[0]==0 && std::memcmp(head.fccType,"sttr",4)==0) {
      auto& sttr = *track.sttr;
      for(const auto& style : sttr.styles){
        auto& stl = owner.style(style.reference);
        this->style = &stl;
        for(auto& band:stl.band) {
          for(auto& r:band.intrument){
            auto& dls = owner.dlsCollection(r.reference);
            Instrument ins;
            ins.dls    = &dls;
            ins.volume = r.header.bVolume/127.f;
            ins.pan    = r.header.bPan/127.f;
            ins.font   = dls.toSoundfont(r.header.dwPatch);

            if(ins.volume<0.f)
              ins.volume=0;

            ins.font.setVolume(ins.volume);
            ins.font.setPan(ins.pan);

            instruments[r.header.dwPChannel] = ins;
            }
          }
        }
      }
    else if(head.ckid[0]==0 && std::memcmp(head.fccType,"cord",4)==0) {
      cordHeader = track.cord->header;
      subchord   = track.cord->subchord;
      }
    }
  index();
  }

void Music::index() {
  if(!style)
    return;
  const Style& stl = *style;
  intern->pptn.resize(stl.patterns.size());
  intern->timeTotal = 0;
  for(size_t i=0;i<stl.patterns.size();++i){
    index(stl,intern->pptn[i],stl.patterns[i]);
    intern->timeTotal += intern->pptn[i].timeTotal;
    }
  }

void Music::index(const Style& stl,Music::PatternInternal &inst, const Pattern &pattern) {
  auto& instument = inst.instruments;
  inst.waves.clear();
  inst.volume.clear();
  inst.dbgName.assign(pattern.info.unam.begin(),pattern.info.unam.end());

  // fill instruments upfront
  for(const auto& pref:pattern.partref) {
    auto pinst = instruments.find(pref.io.wLogicalPartID);
    if(pinst==instruments.end())
      continue;
    if(pref.io.wLogicalPartID!=12)
      ;//continue;
    //if(pref.io.wLogicalPartID==1 || pref.io.wLogicalPartID==0)
    //  continue;
    const Instrument& ins = ((*pinst).second);
    InsInternal& pr=instument[pref.io.wLogicalPartID];
    pr.key    = pref.io.wLogicalPartID;
    pr.font   = ins.font;
    pr.volume = ins.volume;
    pr.pan    = ins.pan;
    }

  for(auto& pref:pattern.partref) {
    auto part = stl.findPart(pref.io.guidPartID);
    if(part==nullptr)
      continue;
    auto i = instument.find(pref.io.wLogicalPartID);
    if(i!=instument.end())
      index(inst,&i->second,*part,0);
    }

  std::sort(inst.waves.begin(),inst.waves.end(),[](const Note& a,const Note& b){
    return a.at<b.at;
    });
  std::sort(inst.volume.begin(),inst.volume.end(),[](const Curve& a,const Curve& b){
    return a.at<b.at;
    });

  inst.timeTotal = inst.waves.size()>0 ? pattern.timeLength() : 0;
  }

void Music::index(Music::PatternInternal &idx, InsInternal* inst, const Style::Part &part,uint64_t timeOffset) {
  for(auto& i:part.notes) {
    uint32_t time = musicOffset(i.mtGridStart, i.nTimeOffset, part.header.timeSig);
    uint32_t dur  = i.mtDuration;
    uint8_t  note = 0;
    if(!musicValueToMIDI(i,cordHeader,subchord,note))
      continue;

    Note rec;
    rec.at       = timeOffset+time;
    rec.duration = dur;
    rec.note     = note;
    rec.velosity = i.bVelocity;
    rec.inst     = inst;

    idx.waves.push_back(rec);
    }

  for(auto& i:part.curves) {
    uint32_t time = musicOffset(i.mtGridStart, i.nTimeOffset, part.header.timeSig);
    uint32_t dur  = i.mtDuration;

    Curve c;
    c.at       = time;
    c.duration = dur;
    c.shape    = i.bCurveShape;
    c.ctrl     = i.bCCData;
    c.startV   = (i.nStartValue&0x7F)/127.f;
    c.endV     = (i.nEndValue  &0x7F)/127.f;
    c.inst     = inst;
    if(i.bEventType==DMUS_CURVET_CCCURVE)
      idx.volume.push_back(c);
    }
  }

void Music::exec(const size_t patternId) const {
  if(!style || patternId>=style->patterns.size())
    return;
  const Style&   stl = *style;
  const Pattern& p   = stl.patterns[patternId];

  Log::i("pattern: ",p.timeLength());
  for(size_t i=0;i<p.partref.size();++i) {
    auto& pref = p.partref[i];
    auto  part = stl.findPart(pref.io.guidPartID);
    if(part==nullptr)
      continue;

    if(part->notes.size()>0 || part->curves.size()>0) {
      std::string st(pref.unfo.unam.begin(),pref.unfo.unam.end());
      Log::i("part: ",i," ",st," partid=",pref.io.wLogicalPartID);
      exec(stl,pref,*part);
      }
    }
  }

void Music::exec(const Style&,const Pattern::PartRef& pref,const Style::Part &part) const {
  for(auto& i:part.notes) {
    uint32_t time = musicOffset(i.mtGridStart, i.nTimeOffset, part.header.timeSig);
    uint8_t  note = 0;
    if(!musicValueToMIDI(i,cordHeader,subchord,note))
      continue;

    auto inst = instruments.find(pref.io.wLogicalPartID);
    if(inst!=instruments.end()) {
      auto w = (*inst).second.dls->findWave(note);
      Log::i("  note:[", note, "] ",time," - ",time+i.mtDuration," var=",i.dwVariation," ",w->info.inam);
      }
    }

  for(auto& i:part.curves) {
    const char* name = "";
    switch(i.bCCData) {
      case Dx8::ExpressionCtl: name = "ExpressionCtl"; break;
      case Dx8::ChannelVolume: name = "ChannelVolume"; break;
      default: name = "?";
      }
    uint32_t time = musicOffset(i.mtGridStart, i.nTimeOffset, part.header.timeSig);

    Log::i("  curve:[", name, "] ",time," - ",time+i.mtDuration," var=",i.dwVariation);
    }
  }
