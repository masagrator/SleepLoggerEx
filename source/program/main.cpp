#include "lib.hpp"
#include "nn/os.hpp"
#include "nn/time.hpp"
#include "nn/fs.hpp"
#include "ltoa.h"

#ifndef WEAK
#define WEAK __attribute__((weak))
#endif

extern "C" {
	FILE * fopen(const char * sNazwaPliku, const char * sTryb ) WEAK;
	size_t fwrite( const void * buffer, size_t size, size_t count, FILE * stream ) WEAK;
}

bool Initialized = false;
bool lock_nx = false;
FILE* file = 0;
bool nx_lock = false;

ptrdiff_t returnInstructionOffset(uintptr_t LR) {
	return LR - exl::util::GetMainModuleInfo().m_Total.m_Start;
}

HOOK_DEFINE_TRAMPOLINE(SleepThread) {

    /* Define the callback for when the function is called. Don't forget to make it static and name it Callback. */
    static void Callback(nn::TimeSpan nanoseconds) {

        /* Call the original function, with the argument always being false. */
		if (!Initialized) {
			nn::fs::MountSdCardForDebug("sdmc");
			file = fopen("sdmc:/SleepDebug.txt", "w");
			if (file)
				Initialized = true;
		}
		else if (nanoseconds.GetNanoSeconds() >= 10'000'000 && nanoseconds.GetNanoSeconds() <= 33'333'333) {
			while (nx_lock) 
				nn::os::SleepThread(nn::TimeSpan::FromNanoSeconds(10000));
			nx_lock = true;
			fwrite("Address0: 0x", 12, 1, file);
			char buffer[34] = "";
			ultoa((unsigned long)returnInstructionOffset((uintptr_t)__builtin_return_address(0)), &buffer[0], 16);
			fwrite(&buffer[0], strlen(&buffer[0]), 1, file);
			fwrite(", Nanoseconds: ", 15, 1, file);
			ultoa(nanoseconds.GetNanoSeconds(), &buffer[0], 10);
			fwrite(&buffer[0], strlen(&buffer[0]), 1, file);
			fwrite("\n", 1, 1, file);
			nx_lock = false;
		}
		
		return Orig(nanoseconds);
    }

};

/* Define hook StubCopyright. Trampoline indicates the original function should be kept. */
/* HOOK_DEFINE_REPLACE can be used if the original function does not need to be kept. */

extern "C" void exl_main(void* x0, void* x1) {
	/* Setup hooking enviroment. */
	exl::hook::Initialize();
	SleepThread::InstallAtFuncPtr(nn::os::SleepThread);

	/* Install the hook at the provided function pointer. Function type is checked against the callback function. */
}

extern "C" NORETURN void exl_exception_entry() {
	/* TODO: exception handling */
	EXL_ABORT(0x420);
}