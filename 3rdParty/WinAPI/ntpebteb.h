#pragma once

#include <minwindef.h>

#ifndef _NTPEBTEB_H
#define _NTPEBTEB_H

typedef LONG32 NTSTATUS;

typedef struct _RTL_USER_PROCESS_PARAMETERS *PRTL_USER_PROCESS_PARAMETERS;
typedef struct _RTL_CRITICAL_SECTION *PRTL_CRITICAL_SECTION;

#define GDI_HANDLE_BUFFER_SIZE32 34
#define GDI_HANDLE_BUFFER_SIZE64 60

#ifndef _WIN64
#define GDI_HANDLE_BUFFER_SIZE GDI_HANDLE_BUFFER_SIZE32
#else
#define GDI_HANDLE_BUFFER_SIZE GDI_HANDLE_BUFFER_SIZE64
#endif

typedef ULONG GDI_HANDLE_BUFFER[GDI_HANDLE_BUFFER_SIZE];

typedef ULONG GDI_HANDLE_BUFFER32[GDI_HANDLE_BUFFER_SIZE32];
typedef ULONG GDI_HANDLE_BUFFER64[GDI_HANDLE_BUFFER_SIZE64];

typedef struct _CLIENT_ID {
	HANDLE UniqueProcess;
	HANDLE UniqueThread;
} CLIENT_ID, *PCLIENT_ID;

typedef struct _PEB_LDR_DATA {
	ULONG Length;
	BOOLEAN Initialized;
	HANDLE SsHandle;
	LIST_ENTRY InLoadOrderModuleList;
	LIST_ENTRY InMemoryOrderModuleList;
	LIST_ENTRY InInitializationOrderModuleList;
	PVOID EntryInProgress;
	BOOLEAN ShutdownInProgress;
	HANDLE ShutdownThreadId;
} PEB_LDR_DATA, *PPEB_LDR_DATA;

typedef struct _UNICODE_STRING {
	USHORT Length;
	USHORT MaximumLength;
	_Field_size_bytes_part_(MaximumLength, Length) PWCH Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

// symbols
typedef struct _PEB {
	BOOLEAN InheritedAddressSpace;                                    // +0x000
	BOOLEAN ReadImageFileExecOptions;                                 // +0x001
	BOOLEAN BeingDebugged;                                            // +0x002
	union {
		BOOLEAN BitField;                                             // +0x003
		struct {
			BOOLEAN ImageUsesLargePages : 1;                          //
			BOOLEAN IsProtectedProcess : 1;                           //
			BOOLEAN IsImageDynamicallyRelocated : 1;                  //
			BOOLEAN SkipPatchingUser32Forwarders : 1;                 //
			BOOLEAN IsPackagedProcess : 1;                            //
			BOOLEAN IsAppContainer : 1;                               //
			BOOLEAN IsProtectedProcessLight : 1;                      //
			BOOLEAN IsLongPathAwareProcess : 1;                       //
		};
	};

	HANDLE Mutant;                                                    // +0x008

	PVOID ImageBaseAddress;                                           // +0x010
	PPEB_LDR_DATA Ldr;                                                // +0x018
	PRTL_USER_PROCESS_PARAMETERS ProcessParameters;                   // +0x020
	PVOID SubSystemData;                                              // +0x028
	PVOID ProcessHeap;                                                // +0x030
	PRTL_CRITICAL_SECTION FastPebLock;                                // +0x038
	PVOID AtlThunkSListPtr;                                           // +0x040
	PVOID IFEOKey;                                                    // +0x048
	union {
		ULONG CrossProcessFlags;                                      // +0x050
		struct {
			ULONG ProcessInJob : 1;                                   //
			ULONG ProcessInitializing : 1;                            //
			ULONG ProcessUsingVEH : 1;                                //
			ULONG ProcessUsingVCH : 1;                                //
			ULONG ProcessUsingFTH : 1;                                //
			ULONG ReservedBits0 : 27;                                 //
		};
	};
	union {
		PVOID KernelCallbackTable;                                    // +0x058
		PVOID UserSharedInfoPtr;                                      // +0x058
	};
	ULONG SystemReserved[1];                                          // +0x060
	ULONG AtlThunkSListPtr32;                                         // +0x064
	PVOID ApiSetMap;                                                  // +0x068
	ULONG TlsExpansionCounter;                                        // +0x070
	PVOID TlsBitmap;                                                  // +0x078
	ULONG TlsBitmapBits[2];                                           // +0x080
	PVOID ReadOnlySharedMemoryBase;                                   // +0x088
	PVOID HotpatchInformation;                                        // +0x090
	PVOID *ReadOnlyStaticServerData;                                  // +0x098
	PVOID AnsiCodePageData;                                           // +0x0A0
	PVOID OemCodePageData;                                            // +0x0A8
	PVOID UnicodeCaseTableData;                                       // +0x0B0

	ULONG NumberOfProcessors;                                         // +0x0B8
	ULONG NtGlobalFlag;                                               // +0x0BC

	LARGE_INTEGER CriticalSectionTimeout;                             // +0x0C0
	SIZE_T HeapSegmentReserve;                                        // +0x0C8
	SIZE_T HeapSegmentCommit;                                         // +0x0D0
	SIZE_T HeapDeCommitTotalFreeThreshold;                            // +0x0D8
	SIZE_T HeapDeCommitFreeBlockThreshold;                            // +0x0E8

	ULONG NumberOfHeaps;                                              // +0x0E8
	ULONG MaximumNumberOfHeaps;                                       // +0x0EC
	PVOID *ProcessHeaps;                                              // +0x0F0

	PVOID GdiSharedHandleTable;                                       // +0x0F8
	PVOID ProcessStarterHelper;                                       // +0x100
	ULONG GdiDCAttributeList;                                         // +0x108

	PRTL_CRITICAL_SECTION LoaderLock;                                 // +0x110

	ULONG OSMajorVersion;                                             // +0x118
	ULONG OSMinorVersion;                                             // +0x11C
	USHORT OSBuildNumber;                                             // +0x120
	USHORT OSCSDVersion;                                              // +0x122
	ULONG OSPlatformId;                                               // +0x124
	ULONG ImageSubsystem;                                             // +0x128
	ULONG ImageSubsystemMajorVersion;                                 // +0x12C
	ULONG ImageSubsystemMinorVersion;                                 // +0x130
	ULONG_PTR ActiveProcessAffinityMask;                              // +0x138
	GDI_HANDLE_BUFFER GdiHandleBuffer;                                // +0x140
	PVOID PostProcessInitRoutine;                                     // +0x1C8

	PVOID TlsExpansionBitmap;                                         // +0x1D0
	ULONG TlsExpansionBitmapBits[32];                                 // +0x1D8

	ULONG SessionId;                                                  // +0x258

	ULARGE_INTEGER AppCompatFlags;                                    // +0x260
	ULARGE_INTEGER AppCompatFlagsUser;                                // +0x268
	PVOID pShimData;                                                  // +0x270
	PVOID AppCompatInfo;                                              // +0x278

	UNICODE_STRING CSDVersion;                                        // +0x280

	PVOID ActivationContextData;                                      // +0x290
	PVOID ProcessAssemblyStorageMap;                                  // +0x298
	PVOID SystemDefaultActivationContextData;                         // +0x2A0
	PVOID SystemAssemblyStorageMap;                                   // +0x2A8

	SIZE_T MinimumStackCommit;                                        // +0x2B0

	PVOID *FlsCallback;                                               // +0x2B8
	LIST_ENTRY FlsListHead;                                           // +0x2C0
	PVOID FlsBitmap;                                                  // +0x2D0
	ULONG FlsBitmapBits[FLS_MAXIMUM_AVAILABLE / (sizeof(ULONG) * 8)]; // +0x2D8
	ULONG FlsHighIndex;                                               // +0x2E8

	PVOID WerRegistrationData;                                        // +0x2F0
	PVOID WerShipAssertPtr;                                           // +0x2F8
	PVOID pContextData;                                               // +0x300
	PVOID pImageHeaderHash;                                           // +0x308
	union {
		ULONG TracingFlags;                                           // +0x310
		struct {
			ULONG HeapTracingEnabled : 1;                             //
			ULONG CritSecTracingEnabled : 1;                          //
			ULONG LibLoaderTracingEnabled : 1;                        //
			ULONG SpareTracingBits : 29;                              //
		};
	};
	ULONGLONG CsrServerReadOnlySharedMemoryBase;                      // +0x318
	PVOID TppWorkerpListLock;                                         // +0x320
	LIST_ENTRY TppWorkerpList;                                        // +0x328
	PVOID WaitOnAddressHashTable[128];                                // +0x338
} PEB, *PPEB;

#define GDI_BATCH_BUFFER_SIZE 310

typedef struct _GDI_TEB_BATCH {
	ULONG Offset;
	ULONG_PTR HDC;
	ULONG Buffer[GDI_BATCH_BUFFER_SIZE];
} GDI_TEB_BATCH, *PGDI_TEB_BATCH;

typedef struct _TEB_ACTIVE_FRAME_CONTEXT {
	ULONG Flags;
	PSTR FrameName;
} TEB_ACTIVE_FRAME_CONTEXT, *PTEB_ACTIVE_FRAME_CONTEXT;

typedef struct _TEB_ACTIVE_FRAME {
	ULONG Flags;
	struct _TEB_ACTIVE_FRAME *Previous;
	PTEB_ACTIVE_FRAME_CONTEXT Context;
} TEB_ACTIVE_FRAME, *PTEB_ACTIVE_FRAME;

typedef struct _RTL_ACTIVATION_CONTEXT_STACK_FRAME {
	struct _RTL_ACTIVATION_CONTEXT_STACK_FRAME *Previous;
	struct _ACTIVATION_CONTEXT                 *ActivationContext;
	ULONG                                       Flags;
} RTL_ACTIVATION_CONTEXT_STACK_FRAME, *PRTL_ACTIVATION_CONTEXT_STACK_FRAME;

typedef struct _ACTIVATION_CONTEXT_STACK {
	ULONG                               Flags;
	ULONG                               NextCookieSequenceNumber;
	RTL_ACTIVATION_CONTEXT_STACK_FRAME *ActiveFrame;
	LIST_ENTRY                          FrameListCache;
} ACTIVATION_CONTEXT_STACK, *PACTIVATION_CONTEXT_STACK;

typedef struct _TEB {
	NT_TIB64 NtTib;                                                   // +0x000

	PVOID EnvironmentPointer;                                         // +0x038
	CLIENT_ID ClientId;                                               // +0x040
	PVOID ActiveRpcHandle;                                            // +0x050
	PVOID ThreadLocalStoragePointer;                                  // +0x058
	PPEB ProcessEnvironmentBlock;                                     // +0x060

	ULONG LastErrorValue;                                             // +0x068
	ULONG CountOfOwnedCriticalSections;                               // +0x06c
	PVOID CsrClientThread;                                            // +0x070
	PVOID Win32ThreadInfo;                                            // +0x078
	ULONG User32Reserved[26];                                         // +0x080
	ULONG UserReserved[5];                                            // +0x0e8
	//UINT8 _PADDING0_[0x4];                                          // +0x0FC
	PVOID WOW32Reserved;                                              // +0x100
	LCID CurrentLocale;                                               // +0x108
	ULONG FpSoftwareStatusRegister;                                   // +0x10c
	PVOID ReservedForDebuggerInstrumentation[16];                     // +0x110
	PVOID SystemReserved1[37];                                        // +0x190
	UCHAR WorkingOnBehalfTicket[8];                                   // +0x2b8
	NTSTATUS ExceptionCode;                                           // +0x2c0
	//UINT8 _PADDING1_[0x4];                                          // +0x2C4

	PACTIVATION_CONTEXT_STACK ActivationContextStackPointer;          // +0x2c8
	ULONG_PTR InstrumentationCallbackSp;                              // +0x2d0
	ULONG_PTR InstrumentationCallbackPreviousPc;                      // +0x2d8
	ULONG_PTR InstrumentationCallbackPreviousSp;                      // +0x2e0
	ULONG TxFsContext;                                                // +0x2e8

	BOOLEAN InstrumentationCallbackDisabled[4];                       // +0x2ec
	GDI_TEB_BATCH GdiTebBatch;                                        // +0x2f0
	CLIENT_ID RealClientId;                                           // +0x7d8
	HANDLE GdiCachedProcessHandle;                                    // +0x7e8
	ULONG GdiClientPID;                                               // +0x7f0
	ULONG GdiClientTID;                                               // +0x7f4
	PVOID GdiThreadLocalInfo;                                         // +0x7f8
	ULONG_PTR Win32ClientInfo[62];                                    // +0x800
	PVOID glDispatchTable[233];                                       // +0x9f0
	ULONG_PTR glReserved1[29];                                        // +0x1138
	PVOID glReserved2;                                                // +0x1220
	PVOID glSectionInfo;                                              // +0x1228
	PVOID glSection;                                                  // +0x1230
	PVOID glTable;                                                    // +0x1238
	PVOID glCurrentRC;                                                // +0x1240
	PVOID glContext;                                                  // +0x1248

	NTSTATUS LastStatusValue;                                         // +0x1250
	//UINT8 _PADDING2_[0x4];                                          // +0x1254
	UNICODE_STRING StaticUnicodeString;                               // +0x1258
	WCHAR StaticUnicodeBuffer[261];                                   // +0x1268
	//UINT8 _PADDING3_[0x6];                                          // +0x1472

	PVOID DeallocationStack;                                          // +0x1478
	PVOID TlsSlots[64];                                               // +0x1480
	LIST_ENTRY TlsLinks;                                              // +0x1680

	PVOID Vdm;                                                        // +0x1690
	PVOID ReservedForNtRpc;                                           // +0x1698
	PVOID DbgSsReserved[2];                                           // +0x16a0

	ULONG HardErrorMode;                                              // +0x16b0
	//UINT8 _PADDING4_[0x4];                                          // +0x16B4
#ifdef _WIN64
	PVOID Instrumentation[11];                                        // +0x16b8
#else
	PVOID Instrumentation[9];
#endif
	GUID ActivityId;                                                  // +0x1710

	PVOID SubProcessTag;                                              // +0x1720
	PVOID PerflibData;                                                // +0x1728
	PVOID EtwTraceData;                                               // +0x1730
	PVOID WinSockData;                                                // +0x1738
	ULONG GdiBatchCount;                                              // +0x1740

	union {
		PROCESSOR_NUMBER CurrentIdealProcessor;                       // +0x1744
		ULONG IdealProcessorValue;                                    // +0x1744
		struct {
			UCHAR ReservedPad0;                                       // +0x1744
			UCHAR ReservedPad1;                                       // +0x1745
			UCHAR ReservedPad2;                                       // +0x1746
			UCHAR IdealProcessor;                                     // +0x1747
		};
	};

	ULONG GuaranteedStackBytes;                                       // +0x1748
	//UINT8 _PADDING5_[0x4];                                          // +0x174C
	PVOID ReservedForPerf;                                            // +0x1750
	PVOID ReservedForOle;                                             // +0x1758
	ULONG WaitingOnLoaderLock;                                        // +0x1760
	//UINT8 _PADDING6_[0x4];                                          // +0x1764
	PVOID SavedPriorityState;                                         // +0x1768
	ULONG_PTR ReservedForCodeCoverage;                                // +0x1770
	PVOID ThreadPoolData;                                             // +0x1778
	PVOID *TlsExpansionSlots;                                         // +0x1780
#ifdef _WIN64
	PVOID DeallocationBStore;                                         // +0x1788
	PVOID BStoreLimit;                                                // +0x1790
#endif
	ULONG MuiGeneration;                                              // +0x1798
	ULONG IsImpersonating;                                            // +0x179c
	PVOID NlsCache;                                                   // +0x17a0
	PVOID pShimData;                                                  // +0x17a8
	USHORT HeapVirtualAffinity;                                       // +0x17b0
	USHORT LowFragHeapDataSlot;                                       // +0x17b2
	//UINT8  _PADDING7_[0x4];                                         // +0x17b4
	HANDLE CurrentTransactionHandle;                                  // +0x17b8
	PTEB_ACTIVE_FRAME ActiveFrame;                                    // +0x17c0
	PVOID FlsData;                                                    // +0x17c8

	PVOID PreferredLanguages;                                         // +0x17d0
	PVOID UserPrefLanguages;                                          // +0x17d8
	PVOID MergedPrefLanguages;                                        // +0x17e0
	ULONG MuiImpersonation;                                           // +0x17e8

	union {
		USHORT CrossTebFlags;                                         // +0x17ec
		USHORT SpareCrossTebBits : 16;                                // +0x17ec
	};

	union {
		USHORT SameTebFlags;                                          // +0x17ee
		struct {
			USHORT SafeThunkCall : 1;                                 // +0x17ee
			USHORT InDebugPrint : 1;                                  // +0x17ee
			USHORT HasFiberData : 1;                                  // +0x17ee
			USHORT SkipThreadAttach : 1;                              // +0x17ee
			USHORT WerInShipAssertCode : 1;                           // +0x17ee
			USHORT RanProcessInit : 1;                                // +0x17ee
			USHORT ClonedThread : 1;                                  // +0x17ee
			USHORT SuppressDebugMsg : 1;                              // +0x17ee
			USHORT DisableUserStackWalk : 1;                          // +0x17ee
			USHORT RtlExceptionAttached : 1;                          // +0x17ee
			USHORT InitialThread : 1;                                 // +0x17ee
			USHORT SessionAware : 1;                                  // +0x17ee
			USHORT LoadOwner : 1;                                     // +0x17ee
			USHORT LoaderWorker : 1;                                  // +0x17ee
			USHORT SpareSameTebBits : 2;                              // +0x17ee
		};
	};

	PVOID TxnScopeEnterCallback;                                      // +0x17f0
	PVOID TxnScopeExitCallback;                                       // +0x17f8
	PVOID TxnScopeContext;                                            // +0x1800
	ULONG LockCount;                                                  // +0x1808
	LONG WowTebOffset;                                                // +0x180c
	PVOID ResourceRetValue;                                           // +0x1810
	PVOID ReservedForWdf;                                             // +0x1818
	ULONGLONG ReservedForCrt;                                         // +0x1820
	GUID EffectiveContainerId;                                        // +0x1828
} TEB, *PTEB, *_PTEB;

typedef struct _FIBER
{
    PVOID FiberData;                                                  // +0x000
    struct _EXCEPTION_REGISTRATION_RECORD *ExceptionList;             // +0x008
    PVOID StackBase;                                                  // +0x010
    PVOID StackLimit;                                                 // +0x018
    PVOID DeallocationStack;                                          // +0x020
    CONTEXT FiberContext;                                             // +0x030
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    PVOID Wx86Tib;                                                    // +0x500
    struct _ACTIVATION_CONTEXT_STACK *ActivationContextStackPointer;  // +0x508
    PVOID FlsData;                                                    // +0x510
    ULONG GuaranteedStackBytes;                                       // +0x518
    ULONG TebFlags;                                                   // +0x51C
    DWORD64 Cookie;                                                   // +0x520
#else
    ULONG GuaranteedStackBytes;
    PVOID FlsData;
    struct _ACTIVATION_CONTEXT_STACK *ActivationContextStackPointer;
#endif
} FIBER, *PFIBER;

#endif