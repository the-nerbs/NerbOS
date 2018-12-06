use16

; note we actually run from START_ADDR initially, and copy to this address first thing.
org 0600h

START_ADDR          =   7C00h                                       ; address we're being loaded to
COPY_ADDR           =   $$                                          ; address to copy ourselves to
SECTOR_SIZE         =   512                                         ; the size of a single sector
COPY_START_ADDR     =   COPY_ADDR + (StartCopy - $$)                ; address to start execution of the copy at
PART_ENTRY0_OFFS    =   01BEh                                       ; offset of partition entry 0
PART_ENTRY0_ADDR    =   COPY_ADDR + PART_ENTRY0_OFFS                ; address of partition entry 0
PART_ENTRY_SIZE     =   16                                          ; size of a partition entry
N_BOOTPART_SECTORS  =   8                                           ; number of sectors to load from the boot partition
N_RESERVED_SECTORS  =   3                                           ; Number of reserved (bootstrap) sectors
SECTORS_PER_TRACK   =   18                                          ; Number of sectors per disk track (cylinder)
N_HEADS             =   2                                           ; Number of disk reader heads
N_ROOT_DIR_ENTRIES  =   32                                          ; Number of root directory entries

MMAP_BASE_ADDR      =   1000h                                       ; Base address of the memory map
KERNEL_LOAD_ADDR    =   10000h                                      ; Address to which we load the kernel image.

; We copy into the memory gap starting at 0x00000500, which only has room for 59 sectors.
assert N_RESERVED_SECTORS < 60

InitialJump:
        jmp         Start
        nop

name                db      "        "
bytes_per_sector    dw      512
sect_per_cluster    db      1
n_reserved_sects    dw      N_RESERVED_SECTORS
n_fats              db      2
n_root_entries      dw      N_ROOT_DIR_ENTRIES
total_sectors       dw      2880
media_type          db      0F0h
sect_per_fat        dw      9
sect_per_track      dw      SECTORS_PER_TRACK
n_heads             dw      N_HEADS
n_hidden_sects      dd      0
n_huge_sectors      dd      0
boot_device         db      0
reserved            db      0
boot_signature      db      29h
volume_id           dd      20171226h
volume_label        db      "MAKE MBR   "
filesys_type        db      "FAT12   "

Start:
        xor         ax, ax                                          ; clear ax
        mov         es, ax                                          ; set the segment registers
        mov         ds, ax                                          ;
        mov         ss, ax                                          ;
        mov         sp, START_ADDR                                  ; set the stack to where we are right now.
                                                                    ; once we copy, this is effectively scratch space.

        mov         si, START_ADDR                                  ; copy ourself to 0000h:0600h
        mov         di, COPY_ADDR                                   ;   |
        mov         cx, SECTOR_SIZE                                 ;   |
        cld                                                         ;  /
        rep movsb                                                   ; /

        push        ax                                              ; push code segment to jump to (= 0)
        push        COPY_START_ADDR                                 ; push offset to jump to
        retf

;   linear address: 0x065A
StartCopy:
        sti
        mov         [boot_device], dl                               ; save off the boot drive number for later
        mov         si, szStarting
        call        PrintString

if N_RESERVED_SECTORS > 1
        mov         ah, 2                                           ; Load in any other sectors we may need.
        mov         al, N_RESERVED_SECTORS-1                        ;   |
        mov         cx, 2                                           ;   |
        mov         dh, 0                                           ;   |
        mov         bx, COPY_ADDR + SECTOR_SIZE                     ;  /
        call        TryInt13Fn2
        jc          ErrorLoading
end if

        call        InitFat12
        jc          ErrorFsInitFailed

        mov         si, sKernelName
        call        FindFileByName

        test        di, di
        jz          ErrorNoKernel

        mov         bx, ((KERNEL_LOAD_ADDR and 0F0000h) shr 4)
        mov         es, bx
        mov         bx, (KERNEL_LOAD_ADDR and 0FFFFh)
        call        LoadPEFile
        jc          ErrorLoading


.enable_a20:
        cli
        call        ClearKeyInQueue                                 ; set 'write next byte to controller port'
        mov         al, 0D1h                                        ;     ps/2 controller function
        out         64h, al                                         ;

        call        ClearKeyInQueue                                 ; set everything but second ps/2 out buffer full
        mov         al, 0DFh                                        ;
        out         60h, al                                         ;

        call        ClearKeyInQueue                                 ;

        sti

.initialize_memory:
        call        LoadMemoryMap                                   ; load the memory map
        jc          ErrorMemInitFailed

.set_video_mode:
        mov         ax, 0003h                                       ; function 0, video mode = 3
        int         10h

.enable_protected_mode:
        lgdt        [GDTValue]                                      ; load our basic GDT
        mov         eax, cr0                                        ; Set the protected mode bit of CR0
        or          al, 1                                           ;  /
        mov         cr0, eax                                        ; /

        jmp         16:ProtectedModeStart                           ; jump to the protected mode code, with cs = 16

use32
ProtectedModeStart:
        mov         ax, 8                                           ; set non-code selectors to use the the full, flat
        mov         ds, ax                                          ; memory model.
        mov         es, ax                                          ;   |
        mov         fs, ax                                          ;   |
        mov         gs, ax                                          ;  /
        mov         ss, ax                                          ; /

        mov         eax, KERNEL_LOAD_ADDR                           ; get the entry point from the PE headers
        call        GetPeEntryPoint32                               ; /

        push        MMAP_BASE_ADDR                                  ; arg0 = memory map pointer
        call        eax                                             ; jump to the actual address of the entry point

        hlt                                                         ; should never be executed


use16
;;=============================================================================
;; utility routines
;;=============================================================================
 ErrorLoading:
        mov         si, szLoadError
        jmp         DisplayError

ErrorFsInitFailed:
        mov         si, szFsInitFailed
        jmp         DisplayError

ErrorMemInitFailed:
        mov         si, szMemInitFailed
        jmp         DisplayError

ErrorNoKernel:
        mov         si, szNoKernel

DisplayError:
        call        PrintString
.error_halt:
        hlt
        jmp         .error_halt

PrintDot:
        push        si
        mov         si, szDot
        call        PrintString
        pop         si
        ret

PrintString:
        pusha
        cld
.print_char:
        lodsb
        test        al, al
        jz          .print_string_done
        mov         bx, 0007h                                       ; CGA page = 0, color = light grey on black
        mov         ah, 0Eh                                         ; interrupt function 0Eh (TTY output)
        int         10h
        jmp         .print_char                                     ; loop until we hit a 0 character
.print_string_done:
        popa
        ret

ClearKeyInQueue:
        sub         cx, cx
        in          al, 64h
        jmp         .delay                                          ; delay a bit so we're sure the io completed
.delay:                                                             ;
        test        al, 2
        jnz         ClearKeyInQueue
        ret



;------------------------------------------------------------------------------
; TryInt13Fn2
;   Attempts a call to INT 13h function 02h five times, or until it succeeds.
;
; Inputs:
;   ax, es:bx, cx, dx   Setup for INT 13h.
;
; Outputs:
;   flags   CF set on error, clear on success.
TryInt13Fn2:
        pusha

        mov         bp, 5

.retry_read:
        dec         bp
        jz          .done

        push        ax
        int         13h
        pop         ax

        jc          .retry_read

.done:
        popa
        ret


; access: [P1XCWA]
;   P = present
;   1 = compulsory bit (may be 0)
;   X = executable
;   C = conforming (code segment) or direction (other segment)
;   W = readable (code segment) or writable (other segment)
;   A = accessed (should be 0, reserved for hardware use)
;
; flags: [PSL]
;   P = Page-granularity
;   S = 32-bit mode
;   L = 64-bit mode
align   8
GDTTable:
        dd      00000000h, 00000000h    ; base=0x00000000, limit=0x00000000, ring=0, access=[ 0    ], flags=[   ]
        dd      0000FFFFh, 00CF9200h    ; base=0x00000000, limit=0x000FFFFF, ring=0, access=[P1  W ], flags=[PS ]
        dd      0000FFFFh, 00CF9A00h    ; base=0x00000000, limit=0x000FFFFF, ring=0, access=[P1X WA], flags=[PS ]
GDTValue:
        dw      $ - GDTTable
        dd      GDTTable


szStarting      db      "Initializing boot.", 0Dh, 0Ah, 0
szFsInitFailed  db      "Failed to initialize file system.", 0Dh, 0Ah, 0
szMemInitFailed db      "Failed to initialize memory", 0Dh, 0Ah, 0
szLoadError     db      "Error loading from disk.", 0Dh, 0Ah, 0
szNoKernel      db      "Failed to find kernel file.", 0Dh, 0Ah, 0
szDot           db      ".", 0

sKernelName     db      "NFLPKRNLexe"
assert ($ - sKernelName) = (8 + 3)


; pad to the marker bytes
times (SECTOR_SIZE - 2 - ($ - $$)) db 00h

MarkerWord:
        db      055h
        db      0AAh


;;=================================================================================================
;; RESERVED SECTOR 2
;;  FAT12 driver code
;;=================================================================================================
use16
FS_BASE_ADDR        =   8000h                               ; Base address of the loaded file system
FS_BASE_DISK_SECTOR =   N_RESERVED_SECTORS                  ; Base sector number of the file system on the disk
N_FATS              =   2                                   ; Number of file tables
N_FAT_SECTORS       =   9                                   ; Number of sectors in a file table
FAT_BYTE_SIZE       =   N_FAT_SECTORS * SECTOR_SIZE         ; Number of bytes in a FAT.

FAT0_BASE_SECTOR    =   0
FAT1_BASE_SECTOR    =   FAT0_BASE_SECTOR + N_FAT_SECTORS

FAT0_BASE_ADDR      =   FS_BASE_ADDR + FAT0_BASE_SECTOR * SECTOR_SIZE   ; Base address of FAT0
FAT1_BASE_ADDR      =   FAT0_BASE_ADDR + FAT_BYTE_SIZE                  ; Base address of FAT1

ROOT_DIR_SECTOR     =   FAT0_BASE_SECTOR + (N_FAT_SECTORS * N_FATS)     ; The root directory sector number.
ROOT_DIR_BASE_ADDR  =   FS_BASE_ADDR + ROOT_DIR_SECTOR * SECTOR_SIZE    ; Base address of the root directory.

DIR_ENTRY_SIZE      =   32                                                  ; size of a directory entry
N_ROOT_DIR_SECTORS  =   (DIR_ENTRY_SIZE * N_ROOT_DIR_ENTRIES) / SECTOR_SIZE ; Number of sectors in the root dir

DATA_BASE_SECTOR    =   ROOT_DIR_SECTOR + N_ROOT_DIR_SECTORS
DATA_BASE_DISK_SECTOR   = FS_BASE_DISK_SECTOR + DATA_BASE_SECTOR

pCurrentDir         dw  ROOT_DIR_BASE_ADDR                          ; Current directory table


;------------------------------------------------------------------------------
; InitFat12
;   Initializes the FAT12 driver 
;
; Inputs:
;   none
;
; Outputs:
;   flags   CF set on error, clear on success.
InitFat12:
        pusha

.READ_SIZE  =   (N_ROOT_DIR_SECTORS + N_FATS * N_FAT_SECTORS)
        mov         bp, .READ_SIZE                          ; bp = # sectors to read
        mov         si, FS_BASE_DISK_SECTOR                 ; si = LBA of sector to read
        mov         bx, FS_BASE_ADDR                        ; bx = base destination address
        mov         dl, [boot_device]

.read_sector:
        call        LbaToChs                                ; 

        mov         ax, 0201h                               ; function 02h, read sector count = 1
        call        TryInt13Fn2                             ; 
        jc          .done                                   ;   on failure, return

        inc         si                                      ; increment LBA to read
        add         bx, SECTOR_SIZE                         ; advance the destination pointer
        dec         bp                                      ; decrement the # of sectors left to read
        jnz         .read_sector                            ;   if none left, return

.done:
        popa
        ret


;------------------------------------------------------------------------------
; FindFileByName
;   Finds a directory entry from the current directory by name.
;
; Inputs:
;   si      Pointer to the an 11 byte array containing the file name.
;
; Outputs:
;   di      Pointer to the directory entry
FindFileByName:
        push        cx
        mov         di, [pCurrentDir]

.check_entry:
        cmp         byte [di], 0E5h             ; if the first character of the name is E5, skip
        je          .next_entry                 ; /

        cmp         byte [di], 0                ; if the first character is 0, we reached the end.
        je          .not_found                  ; /

        push        si                          ; save the pointers across the compare
        push        di                          ; /
        mov         cx, (8+3)                   ; bascially just strncmp(si, di, 8+3)
        repne cmpsb                             ; /
        pop         di                          ; restore the pointers
        pop         si                          ; /

        je          .done                       ; 

.next_entry:
        add         si, DIR_ENTRY_SIZE
        jmp         .check_entry

.not_found:
        xor         di, di

.done:
        pop         cx
        ret


;------------------------------------------------------------------------------
; LoadFirstCluster
;   Loads the first cluster (SECTOR_SIZE bytes) of a file
;
; Inputs:
;   es:bx   Pointer to the memory to load to.
;   di      Pointer to the directory entry of the file to load
;
; Outputs:
;   ax      status code: 0 = success, -1 = FAT corruption, -2 = load error
;   di      A context value used by LoadNextCluster.
LoadFirstCluster:
        push        cx
        push        dx
        push        si

        mov         ax, [di + 1Ah]                      ; set ax = first allocated cluster number

        call        LoadClusterByNumber                 ; load the initial cluster
        jc          .load_failed                        ; 

        call        GetFatEntry                         ; get the FAT0 and FAT1 entries
        jne         .corruption                         ; if they differ, we detected corruption

        mov         di, ax                              ; set di = the next cluster number

.success:
        xor         ax, ax
        jmp         .done

.corruption:
        mov         ax, -1
        jmp         .done

.load_failed:
        mov         ax, -2

.done:
        pop         si
        pop         dx
        pop         cx
        ret


;------------------------------------------------------------------------------
; LoadNextCluster
;   Loads subsequent clusters after a call to LoadFirstCluster
;
; Inputs:
;   es:bx   Pointer to the memory to load to.
;   di      A context value from LoadFirstCluster or the previous call to LoadNextCluster.
;
; Outputs:
;   ax      status code: 0 = success, -1 = FAT corruption, -2 = load error, 1 = finished
;   di      A context value used for the next call to LoadNextCluster.
LoadNextCluster:
        push        cx
        push        dx
        push        si

        cmp         di, 0FFFh
        je          .file_end

        mov         ax, di
        call        LoadClusterByNumber
        jc          .load_failed

        call        GetFatEntry                         ; get the FAT0 and FAT1 entries
        jne         .corruption                         ; if they differ, we detected corruption

        mov         di, ax                              ; set di = the next cluster number

.success:
        xor         ax, ax
        jmp         .done

.corruption:
        mov         ax, -1
        jmp         .done

.load_failed:
        mov         ax, -2
        jmp         .done

.file_end:
        mov         ax, 1

.done:
        pop         si
        pop         dx
        pop         cx
        ret


;------------------------------------------------------------------------------
; LoadClusterByNumber
;   Loads the content of a cluster to memory
;
; Inputs:
;   ax      The cluster number
;   es:bx   The address to load the cluster into
;
; Outputs:
;   flags   Carry set if loading failed, clear if succeeded
LoadClusterByNumber:
        pusha

        add         ax, DATA_BASE_DISK_SECTOR-2         ; note: data sector = cluster - 2 + (base sector of data)
        mov         si, ax                              ; 
        call        LbaToChs                            ; 

        mov         ax, 0201h                           ; function 02h, read sector count = 1
        call        TryInt13Fn2                         ; 

        popa
        ret

;------------------------------------------------------------------------------
; GetFatEntry
;   Gets the allocation data entry at the given cluster.
;
; Inputs:
;   ax      The cluster number
;
; Outputs:
;   ax      The entry value from FAT0
;   dx      The entry value from FAT1
;   flags   The result of comparing the two tables' entries.
GetFatEntry:
        push        si
        push        ax                                  ; save off the original cluster number

        mov         si, ax                              ; si = byte offset of cluster (ax * 1.5)
        shr         ax, 1                               ;  /
        add         si, ax                              ; /

        mov         ax, [si + FAT0_BASE_ADDR]           ; Read the entries from either table
        mov         dx, [si + FAT1_BASE_ADDR]           ; /

        pop         si                                  ; pop original cluster number into si
        test        si, 1                               ; 
        jz          .even_entry                         ; 

.odd_entry:
        shr         ax, 4
        shr         dx, 4
        jmp         .done

.even_entry:
        and         ax, 0FFFh
        and         dx, 0FFFh

.done:
        cmp         ax, dx
        pop         si
        ret

;------------------------------------------------------------------------------
; source: https://stackoverflow.com/a/45495410
;  changes made to fit my own needs
;    Function: LbaToChs
; Description: Translate Logical block address to CHS (Cylinder, Head, Sector).
;              Works for all valid FAT12 compatible disk geometries.
;
;   Resources: http://www.ctyme.com/intr/rb-0607.htm
;              https://en.wikipedia.org/wiki/Logical_block_addressing#CHS_conversion
;              https://stackoverflow.com/q/45434899/3857942
;              Sector    = (LBA mod SPT) + 1
;              Head      = (LBA / SPT) mod HEADS
;              Cylinder  = (LBA / SPT) / HEADS
;
;      Inputs: SI = LBA
;     Outputs: DL = Boot Drive Number
;              DH = Head
;              CH = Cylinder (lower 8 bits of 10-bit cylinder)
;              CL = Sector/Cylinder
;                   Upper 2 bits of 10-bit Cylinders in upper 2 bits of CL
;                   Sector in lower 6 bits of CL
;
;       Notes: Output registers match expectation of Int 13h/AH=2 inputs
;
LbaToChs:
        push        ax                      ; Preserve AX
        mov         ax, si                  ; Copy LBA to AX
        xor         dx, dx                  ; Upper 16-bit of 32-bit value set to 0 for DIV
        div         word [sect_per_track]   ; 32-bit by 16-bit DIV : LBA / SPT
        mov         cl, dl                  ; CL = S = LBA mod SPT
        inc         cl                      ; CL = S = (LBA mod SPT) + 1
        xor         dx, dx                  ; Upper 16-bit of 32-bit value set to 0 for DIV
        div         word [n_heads]          ; 32-bit by 16-bit DIV : (LBA / SPT) / HEADS
        mov         dh, dl                  ; DH = H = (LBA / SPT) mod HEADS
        mov         dl, [boot_device]       ; boot device, not necessary to set but convenient
        mov         ch, al                  ; CH = C(lower 8 bits) = (LBA / SPT) / HEADS
        shl         ah, 6                   ; Store upper 2 bits of 10-bit Cylinder into
        or          cl, ah                  ;     upper 2 bits of Sector (CL)
        pop         ax                      ; Restore scratch registers
        ret


;------------------------------------------------------------------------------
; FileLbaToCluster
;   Prepares loading a cluster of a file (with LoadNextCluster) based on its
;   LBA within the file.
;
; Inputs:
;   ax      The LBA to load
;   di      A pointer to the directory entry.
;
; Outputs:
;   di      A context value used by LoadNextCluster.
;   flags   CF set on error, clear on success
FileLbaToCluster:
        push        ax
        push        cx
        push        dx
        push        si

        mov         cx, ax
        mov         ax, [di + 1Ah]              ; ax = first allocated cluster
        jmp         .loop_check

.advance:
        call        GetFatEntry                 ; Get the FAT entry
        jne         .error                      ;   if we detect corruption fail out.

        cmp         ax, 0FF0h                   ; if it's an end marker
        jge         .error                      ;   exit out of the loop

.loop_check:
        clc                                     ; note: dec does not affect carry flag, so clear it here.
        dec         cx                          ; note: decrement cx even on the first loop since
        js          .done                       ;   the FAT entries contain the *next* cluster
        jmp         .advance                    ; 

.error:
        stc                                     ; set the carry flag

.done:
        mov         di, ax                      ; set the return

        pop         si
        pop         dx
        pop         cx
        pop         ax
        ret

;------------------------------------------------------------------------------
; LoadPEFile
;   Loads a PE file from disk.
;
; Inputs:
;   di      A pointer to the directory entry.
;   es:bx   The base address to load to.
;           Assumed to have ebough space to fit the full PE file.
;
; Outputs:
;   flags   CF set on error, clear on success.
LoadPEFile:
        enter       14, 0   ; locals:
                            ;   bp-2    WORD    base segment of the loaded PE file
                            ;   bp-4    WORD    base address of the loaded PE file (based on es)
                            ;   bp-6    WORD    base address of the nt headers (based on es)
                            ;   bp-8    WORD    number of sections in the file
                            ;   bp-10   WORD    directory entry pointer
                            ;   bp-14   DWORD   the full 32-bit destination address
        pusha

        mov         ax, es
        mov         [bp-2], ax
        mov         [bp-4], bx                  ; save off the PE base address
        mov         [bp-10], di

        call        LoadFirstCluster            ; load the first sector
        jc          .done                       ; 

        mov         ax, [es:bx + 3Ch]           ; ax = e_lfanew
        add         ax, bx                      ; ax = &nt_headers
        mov         [bp-6], ax                  ;   save it off for later

        xchg        ax, bx                      ; note: real mode can't use ax as adressing base...
        mov         cx, [es:bx + 06h]           ; cx = # of sections in PE file
        xchg        bx, ax
        
        mov         [bp-8], cx                  ;   save it off for later
        cmp         cx, 2                       ; if we have more than 2 sections, we need to load
        jle         .load_sections              ;   another sector to have all the section headers

        add         bx, SECTOR_SIZE             ; advance bx and load the next header sector
        call        LoadNextCluster             ; Load the next cluster of the file
        jc          .done                       ; 

.load_sections:
        mov         ax, [bp-6]                  ; ax = &nt_headers
        mov         si, ax                      ;   also set si = &nt_headers
        mov         ax, [es:si+20]              ; ax = sizeof optional headers
        add         si, 24                      ; si = &nt_opt_headers
        add         si, ax                      ; si = &section_headers

        xor         cx, cx                      ; cx = current section index

.load_next_section:
        mov         eax, [es:si+0Ch]            ; eax = RVA of section
        xor         edx, edx                    ; edx = 32-bit address to load to
        mov         dx, [bp-4]                  ;  /
        add         edx, eax                    ; /
        mov         dword [bp-14], edx          ; save it off for later

        mov         bx, dx                      ; bx = low 16bits of dest address
        shr         edx, 16                     ; clear out low bits of edx
        shl         dx, 12                      ;   and lshift to form the destination segment value
        add         dx, [bp-2]
        mov         es, dx                      ; set the segment

        mov         eax, [es:si + 14h]          ; eax = physical offset of section
        shr         eax, 9                      ; ax = LBA of section
        mov         di, [bp-10]                 ; di = &(directory entry)
        call        FileLbaToCluster            ; convert section LBA to its cluster.
        jc          .done                       ; on error, fail out

        mov         eax, [es:si + 10h]          ; eax = physical size of section
        test        eax, eax                    ; if this section is 0-sized
        jz          .check_for_more_sections    ;   just continue checking for more.

.load_section_sector:
        push        eax
        call        LoadNextCluster             ; load the cluster
        jc          .done                       ;   if we failed, error out

        mov         edx, dword [bp-14]          ; compute next sector destination address
        add         edx, SECTOR_SIZE            ; 
        mov         dword [bp-14], edx          ;   and store off the new value for the next loop.
        mov         bx, dx                      ; 
        shr         edx, 16                     ; 
        shl         dx, 12                      ; 
        add         dx, [bp-2]                  ; 
        mov         es, dx                      ; 

        call        PrintDot

        pop         eax
        sub         eax, SECTOR_SIZE            ; keep loading sectors until we reach the end
        jz          .check_for_more_sections    ;   of the section
        jns         .load_section_sector        ; 

.check_for_more_sections:
        add         si, 40                      ; advance si to the next section header.
        inc         cx                          ; increment cx
        cmp         cx, [bp-8]                  ; 
        jl          .load_next_section          ; keep looking at sectors until we reach the end
        clc

.done:
        popa
        leave
        ret

; pad to sector boundary
times (2*SECTOR_SIZE - ($ - $$)) db 00h

MMAP_MAGIC          = 534D4150h

;------------------------------------------------------------------------------
; LoadMemoryMap
;   Loads the BIOS memory map
;
; Inputs:
;   none
;
; Outputs:
;   flags   CF set on error, or clear on success
LoadMemoryMap:
        pusha                                   ; push scratch registers
        mov         bx, es                      ;   and the es segment
        push        bx                          ; 

        xor         ebx, ebx                    ; continuation # (initially 0)
        xor         di, di                      ; es = 0
        mov         es, di                      ; /
        mov         di, MMAP_BASE_ADDR          ; destination address

        mov         dword [di], 0               ; keep the count at the base address
        sub         di, 16                      ; align the first entry to the next 8-byet boundary
                                                ;   after the count field
.invoke:
        add         di, 24
        cmp         di, MMAP_BASE_ADDR+2000h    ; make sure we don't go over the limit. 
        jge         .failure                    ; /

        mov         eax, 0E820h                 ; function number
        mov         ecx, 24                     ; buffer size
        mov         edx, MMAP_MAGIC             ; magic number = "SMAP"
        mov         dword [es:di + 20], 1       ; request ACPI v3 extended attributes
        int         15h
        jc          .failure                    ; on error, fail out

        cmp         eax, MMAP_MAGIC             ; eax = magic number
        jne         .failure                    ; 

        cmp         cx, 20                      ; ecx = returned size of entry
        jl          .failure                    ;   make sure it's valid
        cmp         cx, 24                      ;  /
        jg          .failure                    ; /

        inc         dword [MMAP_BASE_ADDR]      ; increment the count.

        test        ebx, ebx
        jnz         .invoke

.done:
        clc

.failure:
        pop         bx                          ; restore scratch registers and es segment.
        mov         es, bx                      ; 
        popa                                    ; 
        ret


use32
;------------------------------------------------------------------------------
; GetPeEntryPoint32
;   Finds the entry point of a loaded PE file
;
; Inputs:
;   eax     The base address of the loaded PE file.
;
; Outputs:
;   eax     The address of the PE entry point.
GetPeEntryPoint32:
    push    edx
    mov     edx, eax                            ; edx = image base address
    add     edx, dword [eax + 3Ch]              ; edx = e_lfanew
    add     edx, 28h                            ; edx = &(pfnEntryPoint)
    add     eax, [edx]                          ; eax = pfnEntryPoint
    pop     edx
    ret


; pad to sector boundary
times (3*SECTOR_SIZE - ($ - $$)) db 00h

; make sure we have the right number of reserved sectors in our BPB
assert ($-$$) = (SECTOR_SIZE * N_RESERVED_SECTORS)
