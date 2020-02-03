#include <string>
#include <map>
std::map<std::string, uint32_t> vfat_parameters = {
  {"ContReg0",    0x36},
  {"ContReg1",    0x00},
  {"ContReg2",    0x30},
  {"ContReg3",    0x00},
  {"Latency",      156},
  {"IPreampIn",    168},
  {"IPreampFeed",   80},
  {"IPreampOut",   150},
  {"IShaper",      150},
  {"IShaperFeed",  100},
  {"IComp",         90},
  {"VCal",           0},
  {"VThreshold1", 0x64},
  {"VThreshold2", 0x00},
  {"CalPhase",    0x00}
};
