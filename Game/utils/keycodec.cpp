#include "keycodec.h"

#include <cstring>

void KeyCodec::getKeysStr(const std::string& keys, char buf[], size_t bufSz) {
  int32_t k0 = fetch(keys,0,4);
  int32_t k1 = fetch(keys,4,8);

  if(k0==0 && k1==0)
    return;

  char kbuf[2][128] = {};
  bool hasK0 = keyToStr(k0,kbuf[0],128);
  bool hasK1 = keyToStr(k1,kbuf[1],128);

  if(!hasK0 || std::strcmp(kbuf[0],kbuf[1])==0){
    std::snprintf(buf,bufSz,"%s",kbuf[1]);
    return;
    }
  if(!hasK1){
    std::snprintf(buf,bufSz,"%s",kbuf[0]);
    return;
    }

  std::snprintf(buf,bufSz,"%s, %s",kbuf[0],kbuf[1]);
  }

bool KeyCodec::keyToStr(int32_t k, char* buf, size_t bufSz) {
  using namespace Tempest;
  static std::initializer_list<K_Key> keys={
    {Tempest::Event::K_A,       0x1e00},
    {Tempest::Event::K_B,       0x3000},
    {Tempest::Event::K_C,       0x2e00},
    {Tempest::Event::K_D,       0x2000},
    {Tempest::Event::K_E,       0x1200},
    {Tempest::Event::K_F,       0x2100},
    {Tempest::Event::K_G,       0x2200},
    {Tempest::Event::K_H,       0x2300},
    {Tempest::Event::K_I,       0x1700},
    {Tempest::Event::K_J,       0x2400},
    {Tempest::Event::K_K,       0x2500},
    {Tempest::Event::K_L,       0x2600},
    {Tempest::Event::K_M,       0x3200},
    {Tempest::Event::K_N,       0x3100},
    {Tempest::Event::K_O,       0x1800},
    {Tempest::Event::K_P,       0x1900},
    {Tempest::Event::K_Q,       0x1000},
    {Tempest::Event::K_R,       0x1300},
    {Tempest::Event::K_S,       0x1f00},
    {Tempest::Event::K_T,       0x1400},
    {Tempest::Event::K_U,       0x1600},
    {Tempest::Event::K_V,       0x2f00},
    {Tempest::Event::K_W,       0x1100},
    {Tempest::Event::K_X,       0x2d00},
    {Tempest::Event::K_Y,       0x1500},
    {Tempest::Event::K_Z,       0x2c00},

    {Tempest::Event::K_0,       0x8100},
    {Tempest::Event::K_1,       0x7800},
    {Tempest::Event::K_2,       0x7900},
    {Tempest::Event::K_3,       0x7a00},
    {Tempest::Event::K_4,       0x7b00},
    {Tempest::Event::K_5,       0x7c00},
    {Tempest::Event::K_6,       0x7d00},
    {Tempest::Event::K_7,       0x7e00},
    {Tempest::Event::K_8,       0x7f00},
    {Tempest::Event::K_9,       0x8000},

    {Tempest::Event::K_Up,      0xc800},
    {Tempest::Event::K_Down,    0xd000},
    {Tempest::Event::K_Left,    0xcb00},
    {Tempest::Event::K_Right,   0xcd00},

    {Tempest::Event::K_Back,    0x0e00},
    {Tempest::Event::K_Tab,     0x0f00},
    {Tempest::Event::K_Delete,  0xd300},
    {Tempest::Event::K_Space,   0x3900},

    // Left
    {Tempest::Event::K_Control, 0x1d00},
    {Tempest::Event::K_Shift,   0x2a00},
    // Right
    {Tempest::Event::K_Control, 0x9d00},
    {Tempest::Event::K_Shift,   0x3600},
    };

  static std::initializer_list<M_Key> mkeys={
    {Tempest::Event::ButtonLeft,  0x0c02},
    {Tempest::Event::ButtonMid,   0x0e02},
    {Tempest::Event::ButtonRight, 0x0d02},
    };

  for(auto& i:keys)
    if(k==i.code) {
      keyToStr(i.k,buf,bufSz);
      return true;
      }
  for(auto& i:mkeys)
    if(k==i.code) {
      keyToStr(i.k,buf,bufSz);
      return true;
      }
  return false;
  }

void KeyCodec::keyToStr(Tempest::Event::KeyType k, char* buf, size_t bufSz) {
  if(Tempest::Event::K_0<=k && k<=Tempest::Event::K_9) {
    buf[0] = char('0' + (k-Tempest::Event::K_0));
    return;
    }
  if(Tempest::Event::K_A<=k && k<=Tempest::Event::K_Z) {
    buf[0] = char('A' + (k-Tempest::Event::K_A));
    return;
    }

  if(k==Tempest::Event::K_Up) {
    std::strncpy(buf,"CURSOR UP",bufSz);
    return;
    }
  if(k==Tempest::Event::K_Down) {
    std::strncpy(buf,"CURSOR DOWN",bufSz);
    return;
    }
  if(k==Tempest::Event::K_Left) {
    std::strncpy(buf,"CURSOR LEFT",bufSz);
    return;
    }
  if(k==Tempest::Event::K_Right) {
    std::strncpy(buf,"CURSOR RIGHT",bufSz);
    return;
    }
  if(k==Tempest::Event::K_Tab) {
    std::strncpy(buf,"TAB",bufSz);
    return;
    }
  if(k==Tempest::Event::K_Back) {
    std::strncpy(buf,"BACKSPACE",bufSz);
    return;
    }
  if(k==Tempest::Event::K_Space) {
    std::strncpy(buf,"SPACE",bufSz);
    return;
    }
  if(k==Tempest::Event::K_Control) {
    std::strncpy(buf,"CONTROL",bufSz);
    return;
    }
  if(k==Tempest::Event::K_Delete) {
    std::strncpy(buf,"DELETE",bufSz);
    return;
    }
  if(k==Tempest::Event::K_Shift) {
    std::strncpy(buf,"SHIFT",bufSz);
    return;
    }

  buf[0] = '?';
  }

void KeyCodec::keyToStr(Tempest::Event::MouseButton k, char* buf, size_t bufSz) {
  if(k==Tempest::Event::ButtonLeft) {
    std::strncpy(buf,"MOUSE LEFT",bufSz);
    return;
    }
  if(k==Tempest::Event::ButtonMid) {
    std::strncpy(buf,"MOUSE MID",bufSz);
    return;
    }
  if(k==Tempest::Event::ButtonRight) {
    std::strncpy(buf,"MOUSE RIGHT",bufSz);
    return;
    }

  buf[0] = '?';
  }

int KeyCodec::fetch(const std::string& keys, size_t s, size_t e) {
  if(s<keys.size() && e<=keys.size() && s<e) {
    int val=0;
    for(size_t i=s;i<e;++i) {
      char k = keys[i];
      if('0'<=k && k<='9')
        val = val*16+(k-'0'); else
      if('a'<=k && k<='f')
        val = val*16+(k-'a'+10); else
      if('A'<=k && k<='F')
        val = val*16+(k-'A'+10); else
        return 0; //error
      }
    return val;
    }
  return 0;
  }