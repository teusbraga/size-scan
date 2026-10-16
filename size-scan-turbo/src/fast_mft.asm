; ==============================================================================
; SIZE-SCAN TURBO - Low-Level x64 Assembly Engine
; Compiled with Microsoft Macro Assembler x64 (ml64.exe)
; ==============================================================================

.code

; ------------------------------------------------------------------------------
; uint32_t mft_apply_fixup_x64(void *record_1024_bytes)
; RCX = pointer to 1024-byte MFT Record
; Returns: 1 if fixup succeeded, 0 if invalid/corrupt/not a FILE record
; ------------------------------------------------------------------------------
align 16
mft_apply_fixup_x64 proc
    ; 1. Check "FILE" signature (0x454C4946)
    cmp dword ptr [rcx], 454C4946h
    jne invalid_record

    ; 2. Read UpdateSequenceOffset (USO) and UpdateSequenceSize (USS)
    movzx r8d, word ptr [rcx + 4]    ; r8d = UpdateSequenceOffset (USO)
    movzx r9d, word ptr [rcx + 6]    ; r9d = UpdateSequenceSize (USS, in WORDs)

    ; Validate USS: for 1024-byte record (2 sectors of 512 bytes), USS must be at least 3
    cmp r9d, 3
    jl invalid_record

    ; Validate USO bounds: USO + USS*2 <= 1024
    lea eax, [r8d + r9d * 2]
    cmp eax, 1024
    ja invalid_record

    ; r10 = address of USA (Update Sequence Array)
    lea r10, [rcx + r8]
    movzx edx, word ptr [r10]        ; edx = check_word (USA[0])

    ; --- Sector 1 Fixup (offset 510 = 01FEh) ---
    cmp word ptr [rcx + 01FEh], dx
    jne fixup_failed
    mov ax, word ptr [r10 + 2]       ; replacement word 1 (USA[1])
    mov word ptr [rcx + 01FEh], ax

    ; --- Sector 2 Fixup (offset 1022 = 03FEh) ---
    cmp word ptr [rcx + 03FEh], dx
    jne fixup_failed
    mov ax, word ptr [r10 + 4]       ; replacement word 2 (USA[2])
    mov word ptr [rcx + 03FEh], ax

    ; Success
    mov eax, 1
    ret

fixup_failed:
invalid_record:
    xor eax, eax
    ret
mft_apply_fixup_x64 endp


; ------------------------------------------------------------------------------
; uint64_t asm_fast_sum_u64_avx2(const uint64_t *values, size_t count)
; RCX = pointer to uint64_t array
; RDX = count of elements
; Returns: sum of all uint64_t elements
; ------------------------------------------------------------------------------
align 16
asm_fast_sum_u64_avx2 proc
    xor rax, rax
    test rdx, rdx
    jz done_sum

    ; Check if count < 16, fallback to scalar
    cmp rdx, 16
    jb scalar_loop

    vzeroall
    vpxor ymm0, ymm0, ymm0
    vpxor ymm1, ymm1, ymm1
    vpxor ymm2, ymm2, ymm2
    vpxor ymm3, ymm3, ymm3

align 16
simd_loop:
    vpaddq ymm0, ymm0, ymmword ptr [rcx]
    vpaddq ymm1, ymm1, ymmword ptr [rcx + 32]
    vpaddq ymm2, ymm2, ymmword ptr [rcx + 64]
    vpaddq ymm3, ymm3, ymmword ptr [rcx + 96]
    add rcx, 128
    sub rdx, 16
    cmp rdx, 16
    jae simd_loop

    ; Combine 4 YMM accumulators
    vpaddq ymm0, ymm0, ymm1
    vpaddq ymm2, ymm2, ymm3
    vpaddq ymm0, ymm0, ymm2

    ; Horizontal sum of ymm0
    vextracti128 xmm1, ymm0, 1
    vpaddq xmm0, xmm0, xmm1
    vpsrldq xmm1, xmm0, 8
    vpaddq xmm0, xmm0, xmm1
    vmovq rax, xmm0
    vzeroupper

    test rdx, rdx
    jz done_sum

align 16
scalar_loop:
    add rax, qword ptr [rcx]
    add rcx, 8
    dec rdx
    jnz scalar_loop

done_sum:
    ret
asm_fast_sum_u64_avx2 endp


; ------------------------------------------------------------------------------
; uint32_t asm_classify_ext_hash(uint64_t ext_packed_utf16)
; RCX = lowercased 4-wchar packed string (e.g. L".zip")
; Returns: FileCategory enum (CAT_VIDEO=0, CAT_IMAGE=1, CAT_AUDIO=2, 
;                             CAT_ARCHIVE=3, CAT_DOC=4, CAT_EXEC=5, 
;                             CAT_CODE=6, CAT_OTHER=7)
; ------------------------------------------------------------------------------
align 16
asm_classify_ext_hash proc
    ; Common Video: .mp4, .mkv, .avi, .mov, .wmv, .flv, .ts
    ; L".mp4" -> 0x00340070006D002E
    mov rax, 00340070006D002Eh
    cmp rcx, rax
    je is_video
    ; L".mkv" -> 0x0076006B006D002E
    mov rax, 0076006B006D002Eh
    cmp rcx, rax
    je is_video
    ; L".avi" -> 0x006900760061002E
    mov rax, 006900760061002Eh
    cmp rcx, rax
    je is_video
    ; L".mov" -> 0x0076006F006D002E
    mov rax, 0076006F006D002Eh
    cmp rcx, rax
    je is_video
    ; L".wmv" -> 0x0076006D0077002E
    mov rax, 0076006D0077002Eh
    cmp rcx, rax
    je is_video

    ; Common Images: .png, .jpg, .gif, .bmp, .ico
    ; L".png" -> 0x0067006E0070002E
    mov rax, 0067006E0070002Eh
    cmp rcx, rax
    je is_image
    ; L".jpg" -> 0x00670070006A002E
    mov rax, 00670070006A002Eh
    cmp rcx, rax
    je is_image
    ; L".gif" -> 0x006600690067002E
    mov rax, 006600690067002Eh
    cmp rcx, rax
    je is_image
    ; L".bmp" -> 0x0070006D0062002E
    mov rax, 0070006D0062002Eh
    cmp rcx, rax
    je is_image
    ; L".ico" -> 0x006F00630069002E
    mov rax, 006F00630069002Eh
    cmp rcx, rax
    je is_image

    ; Common Audio: .mp3, .wav, .flac, .ogg, .m4a
    ; L".mp3" -> 0x00330070006D002E
    mov rax, 00330070006D002Eh
    cmp rcx, rax
    je is_audio
    ; L".wav" -> 0x007600610077002E
    mov rax, 007600610077002Eh
    cmp rcx, rax
    je is_audio
    ; L".ogg" -> 0x00670067006F002E
    mov rax, 00670067006F002Eh
    cmp rcx, rax
    je is_audio
    ; L".m4a" -> 0x00610034006D002E
    mov rax, 00610034006D002Eh
    cmp rcx, rax
    je is_audio

    ; Common Archives: .zip, .rar, .tar, .iso, .7z
    ; L".zip" -> 0x00700069007A002E
    mov rax, 00700069007A002Eh
    cmp rcx, rax
    je is_archive
    ; L".rar" -> 0x007200610072002E
    mov rax, 007200610072002Eh
    cmp rcx, rax
    je is_archive
    ; L".tar" -> 0x007200610074002E
    mov rax, 007200610074002Eh
    cmp rcx, rax
    je is_archive
    ; L".iso" -> 0x006F00730069002E
    mov rax, 006F00730069002Eh
    cmp rcx, rax
    je is_archive

    ; Common Executables: .exe, .dll, .sys, .msi, .bat
    ; L".exe" -> 0x006500780065002E
    mov rax, 006500780065002Eh
    cmp rcx, rax
    je is_exec
    ; L".dll" -> 0x006C006C0064002E
    mov rax, 006C006C0064002Eh
    cmp rcx, rax
    je is_exec
    ; L".sys" -> 0x007300790073002E
    mov rax, 007300790073002Eh
    cmp rcx, rax
    je is_exec
    ; L".msi" -> 0x00690073006D002E
    mov rax, 00690073006D002Eh
    cmp rcx, rax
    je is_exec
    ; L".bat" -> 0x007400610062002E
    mov rax, 007400610062002Eh
    cmp rcx, rax
    je is_exec

    ; Common Documents: .pdf, .txt, .doc, .xls, .ppt, .csv
    ; L".pdf" -> 0x006600640070002E
    mov rax, 006600640070002Eh
    cmp rcx, rax
    je is_doc
    ; L".txt" -> 0x007400780074002E
    mov rax, 007400780074002Eh
    cmp rcx, rax
    je is_doc
    ; L".doc" -> 0x0063006F0064002E
    mov rax, 0063006F0064002Eh
    cmp rcx, rax
    je is_doc
    ; L".xls" -> 0x0073006C0078002E
    mov rax, 0073006C0078002Eh
    cmp rcx, rax
    je is_doc
    ; L".csv" -> 0x007600730063002E
    mov rax, 007600730063002Eh
    cmp rcx, rax
    je is_doc

    ; Common Code: .cpp, .hpp, .xml, .css, .sql
    ; L".cpp" -> 0x007000700063002E
    mov rax, 007000700063002Eh
    cmp rcx, rax
    je is_code
    ; L".xml" -> 0x006C006D0078002E
    mov rax, 006C006D0078002Eh
    cmp rcx, rax
    je is_code
    ; L".sql" -> 0x006C00710073002E
    mov rax, 006C00710073002Eh
    cmp rcx, rax
    je is_code

    ; Fallback to CAT_OTHER
    mov eax, 7
    ret

is_video:
    mov eax, 0
    ret
is_image:
    mov eax, 1
    ret
is_audio:
    mov eax, 2
    ret
is_archive:
    mov eax, 3
    ret
is_doc:
    mov eax, 4
    ret
is_exec:
    mov eax, 5
    ret
is_code:
    mov eax, 6
    ret
asm_classify_ext_hash endp

end
