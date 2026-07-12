#include <windows.h>
#include <winternl.h>
#include <winnt.h>

// Function pointers
FARPROC (WINAPI *pLoadLibraryA)(
    LPCSTR lpLibFileName
) = NULL;

FARPROC (WINAPI *pGetProcAddress)(
    HMODULE hModule,
    LPCSTR lpProcName
) = NULL;

HANDLE (WINAPI *pGetStdHandle)(
    DWORD nStdHandle
) = NULL;

BOOL (WINAPI *pWriteConsoleA)(
    HANDLE  hConsoleOutput,
    LPCVOID lpBuffer,
    DWORD   nNumberOfCharsToWrite,
    LPDWORD lpNumberOfCharsWritten,
    LPVOID  lpReserved
) = NULL;

static inline int string_equals(const char* str1, const char* str2) {
    while (*str1 && *str2) {
        if (*str1 != *str2) {
            return 0;
        }
        ++str1;
        ++str2;
    }
    return (*str1 == *str2);
}

int main() {
    struct _PEB* peb;
    
    __asm__("mov %%gs:0x60, %%rcx\n\t" // Load peb from RCX register
            "mov %%rcx, %0"
            : "=r" (peb)
            : 
            : "rcx", "memory");
            
    struct _LIST_ENTRY* list = &peb->Ldr->InMemoryOrderModuleList;
    struct _LIST_ENTRY* current = list->Flink->Flink->Flink;
    
    struct _LDR_DATA_TABLE_ENTRY* entry =
        CONTAINING_RECORD(current, struct _LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);
    
    struct _IMAGE_DOS_HEADER* dos_header = (struct _IMAGE_DOS_HEADER*)entry->DllBase;
    struct _IMAGE_NT_HEADERS* nt_header = (struct _IMAGE_NT_HEADERS*)((char*)dos_header + dos_header->e_lfanew);
    struct _IMAGE_OPTIONAL_HEADER64* opt_header = (struct _IMAGE_OPTIONAL_HEADER64*)&nt_header->OptionalHeader;
    
    struct _IMAGE_DATA_DIRECTORY* export_dir =
        &opt_header->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
    
    char* pe_base = (char*)entry->DllBase;
    struct _IMAGE_EXPORT_DIRECTORY* export = (struct _IMAGE_EXPORT_DIRECTORY*)(pe_base + export_dir->VirtualAddress);
    DWORD* addr_rvas = (DWORD*)(pe_base + export->AddressOfFunctions);
    WORD* ordinals = (WORD*)(pe_base + export->AddressOfNameOrdinals);
    DWORD* name_rvas = (DWORD*)(pe_base + export->AddressOfNames);

    DWORD num_names = export->NumberOfNames;

    // First, resolve LoadLibraryA and GetProcAddress from kernel32.dll
    for (DWORD i = 0; i < num_names; ++i) {
        char* name = (char*)(pe_base + name_rvas[i]);
        WORD ordinal = ordinals[i];
        DWORD rva = addr_rvas[ordinal];
        void* func_addr = (void*)(pe_base + rva);

        if (string_equals(name, "LoadLibraryA")) {
            pLoadLibraryA = (FARPROC(WINAPI*)(LPCSTR))func_addr;
        }
        else if (string_equals(name, "GetProcAddress")) {
            pGetProcAddress = (FARPROC(WINAPI*)(HMODULE, LPCSTR))func_addr;
        }
    }

    if (!pLoadLibraryA || !pGetProcAddress) {
        // Couldn't load 'LoadLibraryA' and 'pGetProcAddress'. Your windows is weird.\n"
        return 1;
    }
    // Load kernel32.dll (though it's already loaded, this gets us a HMODULE)
    HMODULE kernel32 = (HMODULE)pLoadLibraryA("kernel32.dll");
    if (kernel32) {
        // Get GetStdHandle
        pGetStdHandle = (HANDLE(WINAPI*)(DWORD))pGetProcAddress(kernel32, "GetStdHandle");
        
        // Get WriteConsoleA
        pWriteConsoleA = (BOOL(WINAPI*)(HANDLE, LPCVOID, DWORD, LPDWORD, LPVOID))
            pGetProcAddress(kernel32, "WriteConsoleA");

        if (pWriteConsoleA && pGetStdHandle) {
            DWORD written;
            pWriteConsoleA(pGetStdHandle(STD_OUTPUT_HANDLE), "WriteConsole without linking!\n", 30, &written, NULL);
        }
    } 


    return 0;
}