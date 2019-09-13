#pragma once
#include "Event.h"
#include "Interface/Core/CPUID.h"
#include "Interface/Core/Frontend.h"
#include "Interface/Core/InternalThreadState.h"
#include "Interface/HLE/Syscalls.h"
#include "Interface/Memory/MemMapper.h"
#include "Interface/IR/PassManager.h"
#include <FEXCore/Config/Config.h>
#include <FEXCore/Core/CPUBackend.h>
#include <stdint.h>

#include <mutex>

namespace FEXCore {
class SyscallHandler;
}

namespace FEXCore::Context {
  enum CoreRunningMode {
    MODE_RUN,
    MODE_SINGLESTEP,
  };

  struct Context {
    friend class FEXCore::SyscallHandler;
    struct {
      bool Multiblock {false};
      bool BreakOnFrontendFailure {true};
      int64_t MaxInstPerBlock {-1LL};
      uint64_t VirtualMemSize {1ULL << 36};
      FEXCore::Config::ConfigCore Core {FEXCore::Config::CONFIG_INTERPRETER};

      // LLVM JIT options
      bool LLVM_MemoryValidation {false};
      bool LLVM_IRValidation {false};
      bool LLVM_PrinterPass {false};
    } Config;

    FEXCore::Memory::MemMapper MemoryMapper;

    std::mutex ThreadCreationMutex;
    uint64_t ThreadID{};
    FEXCore::Core::InternalThreadState* ParentThread;
    std::vector<FEXCore::Core::InternalThreadState*> Threads;
    std::atomic_bool ShouldStop{};
    Event PauseWait;
    bool Running{};
    CoreRunningMode RunningMode {CoreRunningMode::MODE_RUN};
    FEXCore::Frontend::Decoder FrontendDecoder;
    FEXCore::IR::PassManager PassManager;

    FEXCore::CPUIDEmu CPUID;
    FEXCore::SyscallHandler SyscallHandler;
    CustomCPUFactoryType CustomCPUFactory;
    CustomCPUFactoryType FallbackCPUFactory;

    Context();
    ~Context();

    bool InitCore(FEXCore::CodeLoader *Loader);
    FEXCore::Context::ExitReason RunLoop(bool WaitForIdle);
    bool IsPaused() const { return !Running; }
    void Pause();

    // Debugger interface
    void CompileRIP(FEXCore::Core::InternalThreadState *Thread, uint64_t RIP);
    uint64_t GetThreadCount() const;
    FEXCore::Core::RuntimeStats *GetRuntimeStatsForThread(uint64_t Thread);
    FEXCore::Core::CPUState GetCPUState();
    void GetMemoryRegions(std::vector<FEXCore::Memory::MemRegion> *Regions);
    bool GetDebugDataForRIP(uint64_t RIP, FEXCore::Core::DebugData *Data);
    bool FindHostCodeForRIP(uint64_t RIP, uint8_t **Code);

    // XXX:
    // bool FindIRForRIP(uint64_t RIP, FEXCore::IR::IntrusiveIRList **ir);
    // void SetIRForRIP(uint64_t RIP, FEXCore::IR::IntrusiveIRList *const ir);
    FEXCore::Core::ThreadState *GetThreadState();
    void LoadEntryList();

  private:
    void WaitForIdle();
    FEXCore::Core::InternalThreadState* CreateThread(FEXCore::Core::CPUState *NewThreadState, uint64_t ParentTID, uint64_t ChildTID);
    void *MapRegion(FEXCore::Core::InternalThreadState *Thread, uint64_t Offset, uint64_t Size, bool Fixed = false);
    void *ShmBase();
    void MirrorRegion(FEXCore::Core::InternalThreadState *Thread, void *HostPtr, uint64_t Offset, uint64_t Size);
    void CopyMemoryMapping(FEXCore::Core::InternalThreadState *ParentThread, FEXCore::Core::InternalThreadState *ChildThread);
    void InitializeThread(FEXCore::Core::InternalThreadState *Thread);
    void ExecutionThread(FEXCore::Core::InternalThreadState *Thread);
    void RunThread(FEXCore::Core::InternalThreadState *Thread);

    uintptr_t CompileBlock(FEXCore::Core::InternalThreadState *Thread, uint64_t GuestRIP);
    uintptr_t AddBlockMapping(FEXCore::Core::InternalThreadState *Thread, uint64_t Address, void *Ptr);

    FEXCore::CodeLoader *LocalLoader{};

    // Entry Cache
    bool GetFilenameHash(std::string const &Filename, std::string &Hash);
    void AddThreadRIPsToEntryList(FEXCore::Core::InternalThreadState *Thread);
    void SaveEntryList();
    std::set<uint64_t> EntryList;
  };
}