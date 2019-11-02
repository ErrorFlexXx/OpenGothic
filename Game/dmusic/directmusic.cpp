#include "directmusic.h"

#include <fstream>
#include <stdexcept>

#include <Tempest/File>

using namespace Dx8;

DirectMusic::DirectMusic() {
  }

Music DirectMusic::load(const Segment &s) {
  return Music(s,*this);
  }

Music DirectMusic::load(const char *fsgt) {
  Tempest::RFile fin(fsgt);
  std::vector<uint8_t> v(size_t(fin.size()));
  fin.read(&v[0],v.size());

  auto r   = Dx8::Riff(v.data(),v.size());
  auto sgt = Dx8::Segment(r);
  return load(sgt);
  }

const Style &DirectMusic::style(const Reference &id) {
  for(auto& i:styles){
    if(i.first==id.file)
      return i.second;
    }

  Tempest::RFile fin(id.file);
  const size_t length = fin.size();

  std::vector<uint8_t> data(length);
  fin.read(&data[0],data.size());

  Riff  r{data.data(),data.size()};
  Style stl(r);

  styles.emplace_back(id.file,std::move(stl));
  return styles.back().second;
  }

const DlsCollection &DirectMusic::dlsCollection(const Reference &id) {
  return dlsCollection(id.file);
  }

const DlsCollection &DirectMusic::dlsCollection(const std::u16string &file) {
  for(auto& i:dls){
    if(i->first==file)
      return i->second;
    }
  Tempest::RFile fin(file);
  const size_t length = fin.size();

  std::vector<uint8_t> data(length);
  fin.read(reinterpret_cast<char*>(&data[0]),data.size());

  Riff          r{data.data(),data.size()};
  DlsCollection stl(r);

  dls.emplace_back(new std::pair<std::u16string,DlsCollection>(file,std::move(stl)));
  return dls.back()->second;
  }
