// MIT. Check only the arguments, fields and outcome route used by riposte.
#pragma once
#include <Windows.h>
#include <Zydis/Zydis.h>
#include <array>
#include <vector>
#include <string>
#include <stdexcept>
#include <limits>
#include <cstring>
namespace NativeCompatibility {
inline std::string location(uintptr_t v){char s[24];std::snprintf(s,sizeof(s),"0x%llx",static_cast<unsigned long long>(v));return s;}
inline bool accessible(uintptr_t image,uintptr_t rva,size_t size,bool code=false){
 if(!image||!size||rva>UINTPTR_MAX-image||size>UINTPTR_MAX-image-rva)return false;
 auto at=image+rva,end=at+size;
 while(at<end){MEMORY_BASIC_INFORMATION m{};if(!VirtualQuery(reinterpret_cast<void*>(at),&m,sizeof(m))||m.AllocationBase!=reinterpret_cast<void*>(image)||m.State!=MEM_COMMIT||(m.Protect&(PAGE_GUARD|PAGE_NOACCESS)))return false;
 auto p=m.Protect&0xff;bool x=p==PAGE_EXECUTE_READ||p==PAGE_EXECUTE_READWRITE||p==PAGE_EXECUTE_WRITECOPY;
 if(!x&&(code||(p!=PAGE_READONLY&&p!=PAGE_READWRITE&&p!=PAGE_WRITECOPY)))return false;
 auto begin=reinterpret_cast<uintptr_t>(m.BaseAddress);if(m.RegionSize>UINTPTR_MAX-begin||begin+m.RegionSize<=at)return false;at=begin+m.RegionSize;}return true;
}
struct Operand {int type;int reg;int base;int index;int64_t value;unsigned size;};
struct Shape {ZydisMnemonic mnemonic;unsigned count;std::array<Operand,3> operands;};
struct Check {uintptr_t function;const char* dependency;std::vector<Shape> sequence;};
struct Instruction {uintptr_t address;ZydisDecodedInstruction ins{};std::array<ZydisDecodedOperand,ZYDIS_MAX_OPERAND_COUNT> op{};};
inline std::vector<Instruction> instructions(uintptr_t image,uintptr_t entry){
 if(!accessible(image,0,sizeof(IMAGE_DOS_HEADER)))throw std::runtime_error("Riposte PE headers unreadable");
 auto dos=reinterpret_cast<IMAGE_DOS_HEADER*>(image);if(dos->e_magic!=IMAGE_DOS_SIGNATURE||dos->e_lfanew<0||!accessible(image,dos->e_lfanew,sizeof(IMAGE_NT_HEADERS64)))throw std::runtime_error("Riposte PE headers invalid");
 auto nt=reinterpret_cast<IMAGE_NT_HEADERS64*>(image+dos->e_lfanew);auto dir=nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION];
 if(!accessible(image,dir.VirtualAddress,dir.Size)||dir.Size%sizeof(RUNTIME_FUNCTION))throw std::runtime_error("Riposte function-boundary metadata unavailable");
 auto table=reinterpret_cast<RUNTIME_FUNCTION*>(image+dir.VirtualAddress);size_t lo=0,hi=dir.Size/sizeof(RUNTIME_FUNCTION);
 while(lo<hi){auto mid=(lo+hi)/2;if(table[mid].BeginAddress<entry)lo=mid+1;else hi=mid;}
 if(lo>=dir.Size/sizeof(RUNTIME_FUNCTION)||table[lo].BeginAddress!=entry||table[lo].EndAddress<=entry||table[lo].EndAddress-entry>65536)throw std::runtime_error("Riposte required function entry unavailable: "+location(entry));
 auto end=table[lo].EndAddress;if(!accessible(image,entry,end-entry,true))throw std::runtime_error("Riposte required executable range unavailable: "+location(entry));
 ZydisDecoder decoder{};ZydisDecoderInit(&decoder,ZYDIS_MACHINE_MODE_LONG_64,ZYDIS_STACK_WIDTH_64);std::vector<Instruction> result;
 for(auto at=entry;at<end;){Instruction d;d.address=image+at;if(!ZYAN_SUCCESS(ZydisDecoderDecodeFull(&decoder,reinterpret_cast<void*>(image+at),end-at,&d.ins,d.op.data())))throw std::runtime_error("Riposte required instruction cannot be decoded: "+location(at));at+=d.ins.length;if(d.ins.mnemonic!=ZYDIS_MNEMONIC_NOP)result.push_back(d);}
 return result;
}
inline bool matches(uintptr_t image,const Instruction& d,const Shape& s){
 if(d.ins.mnemonic!=s.mnemonic||d.ins.operand_count_visible!=s.count)return false;
 for(unsigned j=0;j<s.count;++j){auto& actual=d.op[j];auto& expected=s.operands[j];if(int(actual.type)!=expected.type||(actual.type!=ZYDIS_OPERAND_TYPE_IMMEDIATE&&actual.size!=expected.size))return false;
 if(actual.type==ZYDIS_OPERAND_TYPE_REGISTER&&int(actual.reg.value)!=expected.reg)return false;
 if(actual.type==ZYDIS_OPERAND_TYPE_MEMORY&&(int(actual.mem.base)!=expected.base||int(actual.mem.index)!=expected.index||actual.mem.disp.value!=expected.value))return false;
 if(actual.type==ZYDIS_OPERAND_TYPE_IMMEDIATE){int64_t value=actual.imm.value.s;if(actual.imm.is_relative){ZyanU64 target{};if(!ZYAN_SUCCESS(ZydisCalcAbsoluteAddress(&d.ins,&actual,d.address,&target)))return false;value=target-image;}if(expected.value!=INT64_MIN&&value!=expected.value)return false;}}
 return true;
}
inline uintptr_t require(uintptr_t image,const Check& check,uintptr_t* start=nullptr,uintptr_t* branch=nullptr){
 auto decoded=instructions(image,check.function);uintptr_t found{};
 for(size_t i=0;i+check.sequence.size()<=decoded.size();++i){bool ok=true;for(size_t j=0;j<check.sequence.size();++j)if(!matches(image,decoded[i+j],check.sequence[j])){ok=false;break;}
 if(ok){if(found)throw std::runtime_error(std::string("Riposte ambiguous ")+check.dependency);auto& last=decoded[i+check.sequence.size()-1];found=last.address+last.ins.length-image;if(start)*start=decoded[i].address-image;if(branch){ZyanU64 target{};if(!ZYAN_SUCCESS(ZydisCalcAbsoluteAddress(&last.ins,&last.op[0],last.address,&target)))throw std::runtime_error("Riposte branch target unavailable");*branch=target-image;}}}
 if(!found)throw std::runtime_error(std::string("Riposte cannot verify ")+check.dependency+" at "+location(check.function));return found;
}
inline void pointer(uintptr_t image,uintptr_t site,uintptr_t expected,const char* reason){
 uintptr_t value{};if(!accessible(image,site,8))throw std::runtime_error(reason);std::memcpy(&value,reinterpret_cast<void*>(image+site),8);if(value!=image+expected)throw std::runtime_error(reason);
}
inline void namedPointer(uintptr_t image,uintptr_t record,const char* name,uintptr_t target){
 if(!accessible(image,record,16))throw std::runtime_error("Riposte named registration unavailable");uintptr_t text{};std::memcpy(&text,reinterpret_cast<void*>(image+record),8);
 if(text<image||!accessible(image,text-image,std::strlen(name)+1)||std::strcmp(reinterpret_cast<const char*>(text),name))throw std::runtime_error(std::string("Riposte registration differs: ")+name);
 pointer(image,record+8,target,"Riposte named function target differs");
}
inline void activeBit(uintptr_t image){
 pointer(image,0x9128e60+56,0x286e260,"Riposte active-component setter differs");
 if(!accessible(image,0x286e260,8,true))throw std::runtime_error("Riposte active-component setter unreadable");
 ZydisDecoder decoder{};ZydisDecoderInit(&decoder,ZYDIS_MACHINE_MODE_LONG_64,ZYDIS_STACK_WIDTH_64);Instruction d;d.address=image+0x286e260;
 if(!ZYAN_SUCCESS(ZydisDecoderDecodeFull(&decoder,reinterpret_cast<void*>(d.address),8,&d.ins,d.op.data())))throw std::runtime_error("Riposte active-component setter undecodable");
 Shape required{ZYDIS_MNEMONIC_OR,2,{{{ZYDIS_OPERAND_TYPE_MEMORY,0,ZYDIS_REGISTER_RCX,ZYDIS_REGISTER_NONE,0x8e,8},{ZYDIS_OPERAND_TYPE_IMMEDIATE,0,0,0,0x10,8}}}};
 if(!matches(image,d,required))throw std::runtime_error("Riposte active-component bit layout differs");
}
inline void direction(uintptr_t image,uintptr_t record,const char* name,uint64_t expected){
 if(!accessible(image,record,16))throw std::runtime_error("Riposte direction enum unreadable");uintptr_t text{};uint64_t value{};std::memcpy(&text,reinterpret_cast<void*>(image+record),8);std::memcpy(&value,reinterpret_cast<void*>(image+record+8),8);
 if(text<image||!accessible(image,text-image,std::strlen(name)+1)||std::strcmp(reinterpret_cast<const char*>(text),name)||value!=expected)throw std::runtime_error("Riposte direction enum differs");
}
inline void property(uintptr_t image,uintptr_t record,const char* name,uint32_t offset){
 if(!accessible(image,record,64))throw std::runtime_error("Riposte property metadata unreadable");uintptr_t text{};uint32_t actual{};std::memcpy(&text,reinterpret_cast<void*>(image+record),8);std::memcpy(&actual,reinterpret_cast<void*>(image+record+0x32),4);
 if(text<image||!accessible(image,text-image,std::strlen(name)+1)||std::strcmp(reinterpret_cast<const char*>(text),name)||actual!=offset)throw std::runtime_error(std::string("Riposte required property layout differs: ")+name);
}
}
