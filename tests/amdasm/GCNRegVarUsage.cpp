/*
 *  CLRadeonExtender - Unofficial OpenCL Radeon Extensions Library
 *  Copyright (C) 2014-2017 Mateusz Szpakowski
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <CLRX/Config.h>
#include <iostream>
#include <sstream>
#include <string>
#include <CLRX/utils/Utilities.h>
#include <CLRX/amdasm/Assembler.h>
#include <CLRX/utils/Containers.h>
#include "../TestUtils.h"

using namespace CLRX;

struct AsmRegVarUsageData
{
    size_t offset;
    const char* regVarName;
    uint16_t rstart, rend;
    AsmRegField regField;
    cxbyte rwFlags;
    cxbyte align;
};

struct GCNRegVarUsageCase
{
    const char* input;
    Array<AsmRegVarUsageData> regVarUsages;
    bool good;
    const char* errorMessages;
};

static const GCNRegVarUsageCase gcnRvuTestCases1Tbl[] =
{
    {   /* 0: skipping test 1 */
        ".regvar rax:s, rbx:s\n"
        ".regvar rax4:s:6, rbx5:s:8\n"
        "s_mov_b32 rax,rbx\n"
        ".space 12\n"
        "s_mov_b32 rax4[2],rbx5[1]\n"
        ".space 134\n"
        "s_mov_b64 rax4[2:3],rbx5[1:2]\n",
        {
            // s_mov_b32 rax,rbx
            { 0, "rax", 0, 1, GCNFIELD_SDST, ASMRVU_WRITE, 1 },
            { 0, "rbx", 0, 1, GCNFIELD_SSRC0, ASMRVU_READ, 1 },
            // s_mov_b32 rax4[2],rbx5[1]
            { 16, "rax4", 2, 3, GCNFIELD_SDST, ASMRVU_WRITE, 1 },
            { 16, "rbx5", 1, 2, GCNFIELD_SSRC0, ASMRVU_READ, 1 },
            // s_mov_b64 rax4[2:3],rbx5[1:2]
            { 154, "rax4", 2, 4, GCNFIELD_SDST, ASMRVU_WRITE, 2 },
            { 154, "rbx5", 1, 3, GCNFIELD_SSRC0, ASMRVU_READ, 2 },
        },
        true, ""
    },
    {   /* 1: skipping test 2 */
        ".regvar rax:s, rbx:s\n"
        ".regvar rax4:s:6, rbx5:s:8\n"
        ".space 200\n"
        "s_mov_b32 rax,rbx\n"
        ".space 12\n"
        "s_mov_b32 rax4[2],rbx5[1]\n"
        ".space 134\n"
        "s_mov_b64 rax4[2:3],rbx5[1:2]\n",
        {
            // s_mov_b32 rax,rbx
            { 200, "rax", 0, 1, GCNFIELD_SDST, ASMRVU_WRITE, 1 },
            { 200, "rbx", 0, 1, GCNFIELD_SSRC0, ASMRVU_READ, 1 },
            // s_mov_b32 rax4[2],rbx5[1]
            { 216, "rax4", 2, 3, GCNFIELD_SDST, ASMRVU_WRITE, 1 },
            { 216, "rbx5", 1, 2, GCNFIELD_SSRC0, ASMRVU_READ, 1 },
            // s_mov_b64 rax4[2:3],rbx5[1:2]
            { 354, "rax4", 2, 4, GCNFIELD_SDST, ASMRVU_WRITE, 2 },
            { 354, "rbx5", 1, 3, GCNFIELD_SSRC0, ASMRVU_READ, 2 },
        },
        true, ""
    },
    {   /* 2: SOP1 encoding */
        ".regvar rax:s, rbx:s\n"
        ".regvar rax4:s:6, rbx5:s:8\n"
        "s_mov_b32 rax,rbx\n"
        "s_mov_b32 rax4[2],rbx5[1]\n"
        "s_mov_b64 rax4[2:3],rbx5[1:2]\n"
        "s_ff1_i32_b64 rbx, rbx5[1:2]\n"
        "s_bitset0_b64 rbx5[3:4],rax\n"
        "s_getpc_b64 rax4[0:1]\n"
        "s_setpc_b64 rax4[2:3]\n"
        "s_cbranch_join rax4[2]\n"
        "s_movrels_b32 rax,rbx\n"
        "s_mov_b32 s23,s31\n"
        "s_mov_b64 s[24:25],s[42:43]\n",
        {
            // s_mov_b32 rax,rbx
            { 0, "rax", 0, 1, GCNFIELD_SDST, ASMRVU_WRITE, 1 },
            { 0, "rbx", 0, 1, GCNFIELD_SSRC0, ASMRVU_READ, 1 },
            // s_mov_b32 rax4[2],rbx5[1]
            { 4, "rax4", 2, 3, GCNFIELD_SDST, ASMRVU_WRITE, 1 },
            { 4, "rbx5", 1, 2, GCNFIELD_SSRC0, ASMRVU_READ, 1 },
            // s_mov_b64 rax4[2:3],rbx5[1:2]
            { 8, "rax4", 2, 4, GCNFIELD_SDST, ASMRVU_WRITE, 2 },
            { 8, "rbx5", 1, 3, GCNFIELD_SSRC0, ASMRVU_READ, 2 },
            // s_ff1_i32_b64 rbx, rbx5[1:2]
            { 12, "rbx", 0, 1, GCNFIELD_SDST, ASMRVU_WRITE, 1 },
            { 12, "rbx5", 1, 3, GCNFIELD_SSRC0, ASMRVU_READ, 2 },
            // s_bitset0_b64 rbx5[3:4],rax
            { 16, "rbx5", 3, 5, GCNFIELD_SDST, ASMRVU_WRITE, 2 },
            { 16, "rax", 0, 1, GCNFIELD_SSRC0, ASMRVU_READ, 1 },
            // s_getpc_b64 rax4[0:1]
            { 20, "rax4", 0, 2, GCNFIELD_SDST, ASMRVU_WRITE, 2 },
            // s_setpc_b64 rax4[2:3]
            { 24, "rax4", 2, 4, GCNFIELD_SSRC0, ASMRVU_READ, 2 },
            // s_cbranch_join rax4[2]
            { 28, "rax4", 2, 3, GCNFIELD_SSRC0, ASMRVU_READ, 1 },
            // s_movrels_b32 rax,rbx
            { 32, "rax", 0, 1, GCNFIELD_SDST, ASMRVU_WRITE, 1 },
            { 32, "rbx", 0, 1, GCNFIELD_SSRC0, ASMRVU_READ, 1 },
            // s_mov_b32 s23,s31
            { 36, nullptr, 23, 24, GCNFIELD_SDST, ASMRVU_WRITE, 0 },
            { 36, nullptr, 31, 32, GCNFIELD_SSRC0, ASMRVU_READ, 0 },
            // s_mov_b64 s[24:25],s[42:43]
            { 40, nullptr, 24, 26, GCNFIELD_SDST, ASMRVU_WRITE, 0 },
            { 40, nullptr, 42, 44, GCNFIELD_SSRC0, ASMRVU_READ, 0 }
        },
        true, ""
    },
    {   /* 3: SOP2 encoding */
        ".regvar rax:s, rbx:s, rdx:s\n"
        ".regvar rax4:s:8, rbx5:s:8, rcx3:s:6\n"
        "s_and_b32 rdx, rax, rbx\n"
        "s_or_b32 rdx, s11, rbx\n"
        "s_xor_b64 rcx3[4:5], rax4[0:1], rbx5[2:3]\n"
        "s_cbranch_g_fork  rcx3[0:1], rax4[2:3]\n"
        "s_and_b32 s46, s21, s62\n"
        "s_xor_b64 s[26:27], s[38:39], s[12:13]\n",
        {
            // s_and_b32 rdx, rax, rbx
            { 0, "rdx", 0, 1, GCNFIELD_SDST, ASMRVU_WRITE, 1 },
            { 0, "rax", 0, 1, GCNFIELD_SSRC0, ASMRVU_READ, 1 },
            { 0, "rbx", 0, 1, GCNFIELD_SSRC1, ASMRVU_READ, 1 },
            // s_or_b32 rdx, s11, rbx
            { 4, "rdx", 0, 1, GCNFIELD_SDST, ASMRVU_WRITE, 1 },
            { 4, nullptr, 11, 12, GCNFIELD_SSRC0, ASMRVU_READ, 0 },
            { 4, "rbx", 0, 1, GCNFIELD_SSRC1, ASMRVU_READ, 1 },
            // s_xor_b64 rcx3[4:5], rax4[0:1], rbx5[2:3]
            { 8, "rcx3", 4, 6, GCNFIELD_SDST, ASMRVU_WRITE, 2 },
            { 8, "rax4", 0, 2, GCNFIELD_SSRC0, ASMRVU_READ, 2 },
            { 8, "rbx5", 2, 4, GCNFIELD_SSRC1, ASMRVU_READ, 2 },
            // s_cbranch_g_fork  rcx3[0:1], rax4[2:3]
            { 12, "rcx3", 0, 2, GCNFIELD_SSRC0, ASMRVU_READ, 2 },
            { 12, "rax4", 2, 4, GCNFIELD_SSRC1, ASMRVU_READ, 2 },
            // s_and_b32 s46, s21, s62
            { 16, nullptr, 46, 47, GCNFIELD_SDST, ASMRVU_WRITE, 0 },
            { 16, nullptr, 21, 22, GCNFIELD_SSRC0, ASMRVU_READ, 0 },
            { 16, nullptr, 62, 63, GCNFIELD_SSRC1, ASMRVU_READ, 0 },
            // s_xor_b64 s[26:27], s[38:39], s[12:13]
            { 20, nullptr, 26, 28, GCNFIELD_SDST, ASMRVU_WRITE, 0 },
            { 20, nullptr, 38, 40, GCNFIELD_SSRC0, ASMRVU_READ, 0 },
            { 20, nullptr, 12, 14, GCNFIELD_SSRC1, ASMRVU_READ, 0 }
        },
        true, ""
    },
    {   /* 4: SOPC encoding */
        ".regvar rax:s, rbx:s\n"
        ".regvar rax4:s:4, rbx5:s:4\n"
        "s_cmp_ge_i32  rax, rbx\n"
        "s_bitcmp0_b64  rbx5[2:3], rax4[3]\n"
        "s_setvskip  rax, rbx5[2]\n"
        "s_cmp_ge_i32  s53, s9\n",
        {
            // s_cmp_ge_i32  rax, rbx
            { 0, "rax", 0, 1, GCNFIELD_SSRC0, ASMRVU_READ, 1 },
            { 0, "rbx", 0, 1, GCNFIELD_SSRC1, ASMRVU_READ, 1 },
            // s_bitcmp0_b64  rbx5[2:3], rax[3]
            { 4, "rbx5", 2, 4, GCNFIELD_SSRC0, ASMRVU_READ, 2 },
            { 4, "rax4", 3, 4, GCNFIELD_SSRC1, ASMRVU_READ, 1 },
            // s_set_vskip  rax, rbx5[2]
            { 8, "rax", 0, 1, GCNFIELD_SSRC0, ASMRVU_READ, 1 },
            { 8, "rbx5", 2, 3, GCNFIELD_SSRC1, ASMRVU_READ, 1 },
            // s_cmp_ge_i32  s53, s9
            { 12, nullptr, 53, 54, GCNFIELD_SSRC0, ASMRVU_READ, 0 },
            { 12, nullptr, 9, 10, GCNFIELD_SSRC1, ASMRVU_READ, 0 }
        },
        true, ""
    },
    {   /* 5: SOPK */
        ".regvar rax:s, rbx:s\n"
        ".regvar rax4:s:4, rbx5:s:4\n"
        "s_cmpk_eq_i32  rbx, 0xd3b9\n"
        "s_addk_i32  rax, 0xd3b9\n"
        "s_cbranch_i_fork rbx5[2:3], xxxx-8\nxxxx:\n"
        "s_getreg_b32 rbx, hwreg(trapsts, 0, 1)\n"
        "s_setreg_b32  hwreg(trapsts, 3, 10), rax\n"
        "s_cmpk_eq_i32  s17, 0xd3b9\n",
        {
            // s_cmpk_eq_i32  rbx, 0xd3b9
            { 0, "rbx", 0, 1, GCNFIELD_SDST, ASMRVU_READ, 1 },
            // s_addk_i32  rax, 0xd3b9
            { 4, "rax", 0, 1, GCNFIELD_SDST, ASMRVU_WRITE, 1 },
            // s_cbranch_i_fork rbx5[2:3], xxxx-8
            { 8, "rbx5", 2, 4, GCNFIELD_SDST, ASMRVU_READ, 2 },
            // s_getreg_b32 rbx, hwreg(trapsts, 0, 1)
            { 12, "rbx", 0, 1, GCNFIELD_SDST, ASMRVU_WRITE, 1 },
            // s_setreg_b32  hwreg(trapsts, 3, 10), rax
            { 16, "rax", 0, 1, GCNFIELD_SDST, ASMRVU_READ, 1 },
            // s_cmpk_eq_i32  s17, 0xd3b9
            { 20, nullptr, 17, 18, GCNFIELD_SDST, ASMRVU_READ, 0 }
        },
        true, ""
    },
    {   /* 6: SMRD */
        ".regvar rax:s, rbx:s\n"
        ".regvar rax4:s:20, rbx5:s:16\n"
        "s_load_dword rbx, rbx5[2:3], 0x5b\n"
        "s_load_dwordx2 rax4[0:1], rbx5[4:5], 0x5b\n"
        "s_load_dwordx4 rax4[0:3], rbx5[6:7], 0x5b\n"
        "s_load_dwordx8 rax4[0:7], rbx5[8:9], 0x5b\n"
        "s_load_dwordx16 rax4[4:19], rbx5[10:11], 0x5b\n"
        "s_load_dword rbx, rbx5[2:3], rbx5[6]\n"
        "s_buffer_load_dwordx4 rax4[0:3], rbx5[8:11], 0x5b\n"
        "s_memtime  rax4[2:3]\n"
        "s_dcache_inv\n"
        "s_load_dwordx2 s[28:29], s[36:37], 0x5b\n"
        "s_buffer_load_dwordx4 s[44:47], s[12:15], 0x5b\n",
        {
            // s_load_dword rbx, rbx5[2:3], 0x5b
            { 0, "rbx", 0, 1, GCNFIELD_SMRD_SDST, ASMRVU_WRITE, 1 },
            { 0, "rbx5", 2, 4, GCNFIELD_SMRD_SBASE, ASMRVU_READ, 2 },
            // s_load_dwordx2 rax4[0:1], rbx5[4:5], 0x5b
            { 4, "rax4", 0, 2, GCNFIELD_SMRD_SDST, ASMRVU_WRITE, 2 },
            { 4, "rbx5", 4, 6, GCNFIELD_SMRD_SBASE, ASMRVU_READ, 2 },
            // s_load_dwordx4 rax4[0:3], rbx5[4:5], 0x5b
            { 8, "rax4", 0, 4, GCNFIELD_SMRD_SDST, ASMRVU_WRITE, 4 },
            { 8, "rbx5", 6, 8, GCNFIELD_SMRD_SBASE, ASMRVU_READ, 2 },
            // s_load_dwordx8 rax4[0:7], rbx5[4:5], 0x5b
            { 12, "rax4", 0, 8, GCNFIELD_SMRD_SDST, ASMRVU_WRITE, 4 },
            { 12, "rbx5", 8, 10, GCNFIELD_SMRD_SBASE, ASMRVU_READ, 2 },
            // s_load_dwordx16 rax4[4:19], rbx5[4:5], 0x5b
            { 16, "rax4", 4, 20, GCNFIELD_SMRD_SDST, ASMRVU_WRITE, 4 },
            { 16, "rbx5", 10, 12, GCNFIELD_SMRD_SBASE, ASMRVU_READ, 2 },
            // s_load_dword rbx, rbx5[2:3], rbx5[6]
            { 20, "rbx", 0, 1, GCNFIELD_SMRD_SDST, ASMRVU_WRITE, 1 },
            { 20, "rbx5", 2, 4, GCNFIELD_SMRD_SBASE, ASMRVU_READ, 2 },
            { 20, "rbx5", 6, 7, GCNFIELD_SMRD_SOFFSET, ASMRVU_READ, 1 },
            // s_buffer_load_dwordx4 rax4[0:3], rbx5[8:11], 0x5b
            { 24, "rax4", 0, 4, GCNFIELD_SMRD_SDST, ASMRVU_WRITE, 4 },
            { 24, "rbx5", 8, 12, GCNFIELD_SMRD_SBASE, ASMRVU_READ, 4 },
            // s_memtime  rax4[2:3]
            { 28, "rax4", 2, 4, GCNFIELD_SMRD_SDST, ASMRVU_WRITE, 2 },
            // s_load_dwordx2 s[28:29], s[36:37], 0x5b
            { 36, nullptr, 28, 30, GCNFIELD_SMRD_SDST, ASMRVU_WRITE, 0 },
            { 36, nullptr, 36, 38, GCNFIELD_SMRD_SBASE, ASMRVU_READ, 0 },
            // s_buffer_load_dwordx4 s[44:47], s[12:15], 0x5b
            { 40, nullptr, 44, 48, GCNFIELD_SMRD_SDST, ASMRVU_WRITE, 0 },
            { 40, nullptr, 12, 16, GCNFIELD_SMRD_SBASE, ASMRVU_READ, 0 }
        },
        true, ""
    },
    {   /* 7: SMEM */
        ".gpu Fiji\n"
        ".regvar rax:s, rbx:s\n"
        ".regvar rax4:s:20, rbx5:s:16\n"
        "s_load_dword rbx, rbx5[2:3], 0x5b\n"
        "s_load_dwordx2 rax4[0:1], rbx5[4:5], 0x5b\n"
        "s_load_dwordx4 rax4[0:3], rbx5[6:7], 0x5b\n"
        "s_load_dwordx8 rax4[0:7], rbx5[8:9], 0x5b\n"
        "s_load_dwordx16 rax4[4:19], rbx5[10:11], 0x5b\n"
        "s_load_dword rbx, rbx5[2:3], rbx5[6]\n"
        "s_buffer_load_dwordx4 rax4[0:3], rbx5[8:11], 0x5b\n"
        "s_memtime  rax4[2:3]\n"
        "s_dcache_inv\n"
        "s_store_dword rbx, rbx5[2:3], 0x5b\n"
        "s_atc_probe  0x32, rax4[12:13], 0xfff5b\n"
        "s_atc_probe_buffer  0x32, rax4[12:15], 0xfff5b\n"
        "s_load_dwordx2 s[28:29], s[36:37], 0x5b\n"
        "s_buffer_load_dwordx4 s[44:47], s[12:15], 0x5b\n",
        {
            // s_load_dword rbx, rbx5[2:3], 0x5b
            { 0, "rbx", 0, 1, GCNFIELD_SMRD_SDST, ASMRVU_WRITE, 1 },
            { 0, "rbx5", 2, 4, GCNFIELD_SMRD_SBASE, ASMRVU_READ, 2 },
            // s_load_dwordx2 rax4[0:1], rbx5[4:5], 0x5b
            { 8, "rax4", 0, 2, GCNFIELD_SMRD_SDST, ASMRVU_WRITE, 2 },
            { 8, "rbx5", 4, 6, GCNFIELD_SMRD_SBASE, ASMRVU_READ, 2 },
            // s_load_dwordx4 rax4[0:3], rbx5[4:5], 0x5b
            { 16, "rax4", 0, 4, GCNFIELD_SMRD_SDST, ASMRVU_WRITE, 4 },
            { 16, "rbx5", 6, 8, GCNFIELD_SMRD_SBASE, ASMRVU_READ, 2 },
            // s_load_dwordx8 rax4[0:7], rbx5[4:5], 0x5b
            { 24, "rax4", 0, 8, GCNFIELD_SMRD_SDST, ASMRVU_WRITE, 4 },
            { 24, "rbx5", 8, 10, GCNFIELD_SMRD_SBASE, ASMRVU_READ, 2 },
            // s_load_dwordx16 rax4[4:19], rbx5[4:5], 0x5b
            { 32, "rax4", 4, 20, GCNFIELD_SMRD_SDST, ASMRVU_WRITE, 4 },
            { 32, "rbx5", 10, 12, GCNFIELD_SMRD_SBASE, ASMRVU_READ, 2 },
            // s_load_dword rbx, rbx5[2:3], rbx5[6]
            { 40, "rbx", 0, 1, GCNFIELD_SMRD_SDST, ASMRVU_WRITE, 1 },
            { 40, "rbx5", 2, 4, GCNFIELD_SMRD_SBASE, ASMRVU_READ, 2 },
            { 40, "rbx5", 6, 7, GCNFIELD_SMRD_SOFFSET, ASMRVU_READ, 1 },
            // s_buffer_load_dwordx4 rax4[0:3], rbx5[8:11], 0x5b
            { 48, "rax4", 0, 4, GCNFIELD_SMRD_SDST, ASMRVU_WRITE, 4 },
            { 48, "rbx5", 8, 12, GCNFIELD_SMRD_SBASE, ASMRVU_READ, 4 },
            // s_memtime  rax4[2:3]
            { 56, "rax4", 2, 4, GCNFIELD_SMRD_SDST, ASMRVU_WRITE, 2 },
            // s_store_dword rbx, rbx5[2:3], 0x5b\n
            { 72, "rbx", 0, 1, GCNFIELD_SMRD_SDST, ASMRVU_READ, 1 },
            { 72, "rbx5", 2, 4, GCNFIELD_SMRD_SBASE, ASMRVU_READ, 2 },
            // s_atc_probe  0x32, rax4[12:13], 0xfff5b
            { 80, "rax4", 12, 14, GCNFIELD_SMRD_SBASE, ASMRVU_READ, 2 },
            // s_atc_probe_buffer 0x32, rax4[12:13], 0xfff5b
            { 88, "rax4", 12, 16, GCNFIELD_SMRD_SBASE, ASMRVU_READ, 4 },
            // s_load_dwordx2 s[28:29], s[36:37], 0x5b
            { 96, nullptr, 28, 30, GCNFIELD_SMRD_SDST, ASMRVU_WRITE, 0 },
            { 96, nullptr, 36, 38, GCNFIELD_SMRD_SBASE, ASMRVU_READ, 0 },
            // s_buffer_load_dwordx4 s[44:47], s[12:15], 0x5b
            { 104, nullptr, 44, 48, GCNFIELD_SMRD_SDST, ASMRVU_WRITE, 0 },
            { 104, nullptr, 12, 16, GCNFIELD_SMRD_SBASE, ASMRVU_READ, 0 }
        },
        true, ""
    },
    {   /* 8: VOP2 */
        ".regvar rax:v, rbx:v, rex:v\n"
        ".regvar rax2:v:8, rbx4:v:8, rex5:v:10\n"
        ".regvar srex:s, srdx3:s:6, srbx:s\n"
        "v_sub_f32  rex, rax, rbx\n"
        "v_sub_f32  rex, srex, rbx\n"
        "v_cndmask_b32 rex, rax, rbx, vcc\n"
        "v_addc_u32  rex, vcc, rax, rbx, vcc\n"
        "v_readlane_b32 srex, rax2[3], srdx3[4]\n"
        "v_writelane_b32 rax, rax2[4], srdx3[3]\n"
        "v_sub_f32  rex, rax, rbx vop3\n"
        "v_readlane_b32 srex, rax2[3], srdx3[4] vop3\n"
        "v_addc_u32  rex, srdx3[0:1], rax, rbx, srdx3[2:3]\n"
        "v_sub_f32  rex, rax, srbx\n"
        "v_sub_f32  v46, v42, v22\n"
        "v_sub_f32  v46, s42, v22\n"
        "v_addc_u32  v17, vcc, v53, v25, vcc\n"
        "v_readlane_b32 s45, v37, s14\n"
        "v_addc_u32  v67, s[4:5], v58, v13, s[18:19]\n"
        "v_readlane_b32 s51, v26, s37 vop3\n"
        // extra v_mac_f32
        "v_mac_f32  rex, rax, rbx\n"
        "v_mac_legacy_f32  rex, rax, rbx\n"
        "v_mac_f32  rex, rax, rbx vop3\n"
        "v_mac_f32  v46, v42, v22\n",
        {
            // v_sub_f32  rex, rax, rbx
            { 0, "rex", 0, 1, GCNFIELD_VOP_VDST, ASMRVU_WRITE, 1 },
            { 0, "rax", 0, 1, GCNFIELD_VOP_SRC0, ASMRVU_READ, 1 },
            { 0, "rbx", 0, 1, GCNFIELD_VOP_VSRC1, ASMRVU_READ, 1 },
            // v_sub_f32  rex, srex, rbx
            { 4, "rex", 0, 1, GCNFIELD_VOP_VDST, ASMRVU_WRITE, 1 },
            { 4, "srex", 0, 1, GCNFIELD_VOP_SRC0, ASMRVU_READ, 1 },
            { 4, "rbx", 0, 1, GCNFIELD_VOP_VSRC1, ASMRVU_READ, 1 },
            // v_cndmask_b32 rex, rax, rbx, vcc
            { 8, "rex", 0, 1, GCNFIELD_VOP_VDST, ASMRVU_WRITE, 1 },
            { 8, "rax", 0, 1, GCNFIELD_VOP_SRC0, ASMRVU_READ, 1 },
            { 8, "rbx", 0, 1, GCNFIELD_VOP_VSRC1, ASMRVU_READ, 1 },
            // v_addc_u32  rex, vcc, rax, rbx, vcc
            { 12, "rex", 0, 1, GCNFIELD_VOP_VDST, ASMRVU_WRITE, 1 },
            { 12, "rax", 0, 1, GCNFIELD_VOP_SRC0, ASMRVU_READ, 1 },
            { 12, "rbx", 0, 1, GCNFIELD_VOP_VSRC1, ASMRVU_READ, 1 },
            // v_readlane_b32 srex, rax2[3], srdx3[4]
            { 16, "srex", 0, 1, GCNFIELD_VOP_SDST, ASMRVU_WRITE, 1 },
            { 16, "rax2", 3, 4, GCNFIELD_VOP_SRC0, ASMRVU_READ, 1 },
            { 16, "srdx3", 4, 5, GCNFIELD_VOP_SSRC1, ASMRVU_READ, 1 },
            // v_writelane_b32 rax, rax2[4], srdx3[3]
            { 20, "rax", 0, 1, GCNFIELD_VOP_VDST, ASMRVU_WRITE, 1 },
            { 20, "rax2", 4, 5, GCNFIELD_VOP_SRC0, ASMRVU_READ, 1 },
            { 20, "srdx3", 3, 4, GCNFIELD_VOP_SSRC1, ASMRVU_READ, 1 },
            /* vop3 encoding */
            // v_sub_f32  rex, rax, rbx vop3
            { 24, "rex", 0, 1, GCNFIELD_VOP3_VDST, ASMRVU_WRITE, 1 },
            { 24, "rax", 0, 1, GCNFIELD_VOP3_SRC0, ASMRVU_READ, 1 },
            { 24, "rbx", 0, 1, GCNFIELD_VOP3_SRC1, ASMRVU_READ, 1 },
            // v_readlane_b32 srex, rax2[3], srdx3[4] vop3
            { 32, "srex", 0, 1, GCNFIELD_VOP3_SDST0, ASMRVU_WRITE, 1 },
            { 32, "rax2", 3, 4, GCNFIELD_VOP3_SRC0, ASMRVU_READ, 1 },
            { 32, "srdx3", 4, 5, GCNFIELD_VOP3_SRC1, ASMRVU_READ, 1 },
            // v_addc_u32  rex, srex, rax, rbx, srdx3[1]
            { 40, "rex", 0, 1, GCNFIELD_VOP3_VDST, ASMRVU_WRITE, 1 },
            { 40, "srdx3", 0, 2, GCNFIELD_VOP3_SDST1, ASMRVU_WRITE, 1 },
            { 40, "rax", 0, 1, GCNFIELD_VOP3_SRC0, ASMRVU_READ, 1 },
            { 40, "rbx", 0, 1, GCNFIELD_VOP3_SRC1, ASMRVU_READ, 1 },
            { 40, "srdx3", 2, 4, GCNFIELD_VOP3_SSRC, ASMRVU_READ, 1 },
            // v_sub_f32  rex, rax, srbx
            { 48, "rex", 0, 1, GCNFIELD_VOP3_VDST, ASMRVU_WRITE, 1 },
            { 48, "rax", 0, 1, GCNFIELD_VOP3_SRC0, ASMRVU_READ, 1 },
            { 48, "srbx", 0, 1, GCNFIELD_VOP3_SRC1, ASMRVU_READ, 1 },
            // v_sub_f32  v46, v42, v22
            { 56, nullptr, 256+46, 256+47, GCNFIELD_VOP_VDST, ASMRVU_WRITE, 0 },
            { 56, nullptr, 256+42, 256+43, GCNFIELD_VOP_SRC0, ASMRVU_READ, 0 },
            { 56, nullptr, 256+22, 256+23, GCNFIELD_VOP_VSRC1, ASMRVU_READ, 0 },
            // v_sub_f32  v46, s42, v22
            { 60, nullptr, 256+46, 256+47, GCNFIELD_VOP_VDST, ASMRVU_WRITE, 0 },
            { 60, nullptr, 42, 43, GCNFIELD_VOP_SRC0, ASMRVU_READ, 0 },
            { 60, nullptr, 256+22, 256+23, GCNFIELD_VOP_VSRC1, ASMRVU_READ, 0 },
            // v_addc_u32  v17, vcc, v53, v25, vcc
            { 64, nullptr, 256+17, 256+18, GCNFIELD_VOP_VDST, ASMRVU_WRITE, 0 },
            { 64, nullptr, 256+53, 256+54, GCNFIELD_VOP_SRC0, ASMRVU_READ, 0 },
            { 64, nullptr, 256+25, 256+26, GCNFIELD_VOP_VSRC1, ASMRVU_READ, 0 },
            // v_readlane_b32 s45, v37, s14
            { 68, nullptr, 45, 46, GCNFIELD_VOP_SDST, ASMRVU_WRITE, 0 },
            { 68, nullptr, 256+37, 256+38, GCNFIELD_VOP_SRC0, ASMRVU_READ, 0 },
            { 68, nullptr, 14, 15, GCNFIELD_VOP_SSRC1, ASMRVU_READ, 0 },
            // v_addc_u32  v67, s[4:5], v58, v13, s[18:19]
            { 72, nullptr, 256+67, 256+68, GCNFIELD_VOP3_VDST, ASMRVU_WRITE, 0 },
            { 72, nullptr, 4, 6, GCNFIELD_VOP3_SDST1, ASMRVU_WRITE, 0 },
            { 72, nullptr, 256+58, 256+59, GCNFIELD_VOP3_SRC0, ASMRVU_READ, 0 },
            { 72, nullptr, 256+13, 256+14, GCNFIELD_VOP3_SRC1, ASMRVU_READ, 0 },
            { 72, nullptr, 18, 20, GCNFIELD_VOP3_SSRC, ASMRVU_READ, 0 },
            // v_readlane_b32 s51, v26, s37 vop3
            { 80, nullptr, 51, 52, GCNFIELD_VOP3_SDST0, ASMRVU_WRITE, 0 },
            { 80, nullptr, 256+26, 256+27, GCNFIELD_VOP3_SRC0, ASMRVU_READ, 0 },
            { 80, nullptr, 37, 38, GCNFIELD_VOP3_SRC1, ASMRVU_READ, 0 },
            // v_mac_f32  rex, rax, rbx
            { 88, "rex", 0, 1, GCNFIELD_VOP_VDST, ASMRVU_WRITE|ASMRVU_READ, 1 },
            { 88, "rax", 0, 1, GCNFIELD_VOP_SRC0, ASMRVU_READ, 1 },
            { 88, "rbx", 0, 1, GCNFIELD_VOP_VSRC1, ASMRVU_READ, 1 },
            // v_mac_legacy_f32  rex, rax, rbx
            { 92, "rex", 0, 1, GCNFIELD_VOP_VDST, ASMRVU_WRITE|ASMRVU_READ, 1 },
            { 92, "rax", 0, 1, GCNFIELD_VOP_SRC0, ASMRVU_READ, 1 },
            { 92, "rbx", 0, 1, GCNFIELD_VOP_VSRC1, ASMRVU_READ, 1 },
            // v_mac_f32  rex, rax, rbx vop3
            { 96, "rex", 0, 1, GCNFIELD_VOP3_VDST, ASMRVU_WRITE|ASMRVU_READ, 1 },
            { 96, "rax", 0, 1, GCNFIELD_VOP3_SRC0, ASMRVU_READ, 1 },
            { 96, "rbx", 0, 1, GCNFIELD_VOP3_SRC1, ASMRVU_READ, 1 },
            // v_mac_f32  v46, v42, v22
            { 104, nullptr, 256+46, 256+47, GCNFIELD_VOP_VDST,
                        ASMRVU_WRITE|ASMRVU_READ, 0 },
            { 104, nullptr, 256+42, 256+43, GCNFIELD_VOP_SRC0, ASMRVU_READ, 0 },
            { 104, nullptr, 256+22, 256+23, GCNFIELD_VOP_VSRC1, ASMRVU_READ, 0 }
        },
        true, ""
    },
    {   /* 9: VOP1 */
        ".regvar rax:v, rbx:v, rex:v\n"
        ".regvar rax2:v:8, rbx4:v:8, rex5:v:10\n"
        ".regvar srex:s, srdx3:s:6, srbx:s\n"
        "v_cvt_f32_i32 rex, rax\n"
        "v_cvt_f32_i32 rex, srbx\n"
        "v_rcp_f64 rax2[2:3], rbx4[5:6]\n"
        "v_rcp_f64 rax2[2:3], srdx3[1:2]\n"
        "v_readfirstlane_b32 srex, rbx\n"
        "v_nop\n"
        "v_cvt_i32_f64 rbx, rax2[3:4]\n"
        "v_cvt_f32_i32 rex, rax vop3\n"
        "v_cvt_f32_i32 rex, srbx vop3\n"
        "v_rcp_f64 rax2[2:3], rbx4[5:6] vop3\n"
        "v_rcp_f64 rax2[2:3], srdx3[1:2] vop3\n"
        "v_readfirstlane_b32 srex, rbx vop3\n"
        "v_cvt_f32_i32 v43, v147\n"
        "v_cvt_f32_i32 v51, s19\n"
        "v_rcp_f64 v[72:73], v[27:28]\n"
        "v_rcp_f64 v[72:73], s[27:28]\n"
        "v_readfirstlane_b32 s35, v91\n"
        "v_rcp_f64 v[55:56], v[87:88] vop3\n"
        "v_cvt_f32_i32 v43, v147 vop3\n",
        {
            // v_cvt_f32_i32 rex, rax
            { 0, "rex", 0, 1, GCNFIELD_VOP_VDST, ASMRVU_WRITE, 1 },
            { 0, "rax", 0, 1, GCNFIELD_VOP_SRC0, ASMRVU_READ, 1 },
            // v_cvt_f32_i32 rex, srbx
            { 4, "rex", 0, 1, GCNFIELD_VOP_VDST, ASMRVU_WRITE, 1 },
            { 4, "srbx", 0, 1, GCNFIELD_VOP_SRC0, ASMRVU_READ, 1 },
            // v_rcp_f64 rax2[2:3], rbx4[6:7]
            { 8, "rax2", 2, 4, GCNFIELD_VOP_VDST, ASMRVU_WRITE, 1 },
            { 8, "rbx4", 5, 7, GCNFIELD_VOP_SRC0, ASMRVU_READ, 1 },
            // v_rcp_f64 rax2[2:3], srdx3[1:2]
            { 12, "rax2", 2, 4, GCNFIELD_VOP_VDST, ASMRVU_WRITE, 1 },
            { 12, "srdx3", 1, 3, GCNFIELD_VOP_SRC0, ASMRVU_READ, 1 },
            // v_readfirstlane_b32 srex, rbx
            { 16, "srex", 0, 1, GCNFIELD_VOP_SDST, ASMRVU_WRITE, 1 },
            { 16, "rbx", 0, 1, GCNFIELD_VOP_SRC0, ASMRVU_READ, 1 },
            // v_cvt_i32_f64 rbx, rax2[3:4]
            { 24, "rbx", 0, 1, GCNFIELD_VOP_VDST, ASMRVU_WRITE, 1 },
            { 24, "rax2", 3, 5, GCNFIELD_VOP_SRC0, ASMRVU_READ, 1 },
            // v_cvt_f32_i32 rex, rax vop3
            { 28, "rex", 0, 1, GCNFIELD_VOP3_VDST, ASMRVU_WRITE, 1 },
            { 28, "rax", 0, 1, GCNFIELD_VOP3_SRC0, ASMRVU_READ, 1 },
            // v_cvt_f32_i32 rex, srbx vop3
            { 36, "rex", 0, 1, GCNFIELD_VOP3_VDST, ASMRVU_WRITE, 1 },
            { 36, "srbx", 0, 1, GCNFIELD_VOP3_SRC0, ASMRVU_READ, 1 },
            // v_rcp_f64 rax2[2:3], rbx4[6:7] vop3
            { 44, "rax2", 2, 4, GCNFIELD_VOP3_VDST, ASMRVU_WRITE, 1 },
            { 44, "rbx4", 5, 7, GCNFIELD_VOP3_SRC0, ASMRVU_READ, 1 },
            // v_rcp_f64 rax2[2:3], srdx3[1:2] vop3
            { 52, "rax2", 2, 4, GCNFIELD_VOP3_VDST, ASMRVU_WRITE, 1 },
            { 52, "srdx3", 1, 3, GCNFIELD_VOP3_SRC0, ASMRVU_READ, 1 },
            // v_readfirstlane_b32 srex, rbx vop3
            { 60, "srex", 0, 1, GCNFIELD_VOP3_SDST0, ASMRVU_WRITE, 1 },
            { 60, "rbx", 0, 1, GCNFIELD_VOP3_SRC0, ASMRVU_READ, 1 },
            // v_cvt_f32_i32 v43, v147
            { 68, nullptr, 256+43, 256+44, GCNFIELD_VOP_VDST, ASMRVU_WRITE, 0 },
            { 68, nullptr, 256+147, 256+148, GCNFIELD_VOP_SRC0, ASMRVU_READ, 0 },
            // v_cvt_f32_i32 v51, s19
            { 72, nullptr, 256+51, 256+52, GCNFIELD_VOP_VDST, ASMRVU_WRITE, 0 },
            { 72, nullptr, 19, 20, GCNFIELD_VOP_SRC0, ASMRVU_READ, 0 },
            // v_rcp_f64 v[72:73], v[27:28]
            { 76, nullptr, 256+72, 256+74, GCNFIELD_VOP_VDST, ASMRVU_WRITE, 0 },
            { 76, nullptr, 256+27, 256+29, GCNFIELD_VOP_SRC0, ASMRVU_READ, 0 },
            // v_rcp_f64 v[72:73], s[27:28]
            { 80, nullptr, 256+72, 256+74, GCNFIELD_VOP_VDST, ASMRVU_WRITE, 0 },
            { 80, nullptr, 27, 29, GCNFIELD_VOP_SRC0, ASMRVU_READ, 0 },
            // v_readfirstlane_b32 s35, v91
            { 84, nullptr, 35, 36, GCNFIELD_VOP_SDST, ASMRVU_WRITE, 0 },
            { 84, nullptr, 256+91, 256+92, GCNFIELD_VOP_SRC0, ASMRVU_READ, 0 },
            // v_rcp_f64 v[55:56], v[87:88] vop3
            { 88, nullptr, 256+55, 256+57, GCNFIELD_VOP3_VDST, ASMRVU_WRITE, 0 },
            { 88, nullptr, 256+87, 256+89, GCNFIELD_VOP3_SRC0, ASMRVU_READ, 0 },
            // v_cvt_f32_i32 v43, v147
            { 96, nullptr, 256+43, 256+44, GCNFIELD_VOP3_VDST, ASMRVU_WRITE, 0 },
            { 96, nullptr, 256+147, 256+148, GCNFIELD_VOP3_SRC0, ASMRVU_READ, 0 }
        },
        true, ""
    },
    {   /* 10: VOPC */
        ".regvar rax:v, rbx:v, rex:v\n"
        ".regvar rax2:v:8, rbx4:v:8, rex5:v:10\n"
        ".regvar srex:s, srdx3:s:6, srbx:s\n"
        "v_cmp_gt_u32 vcc, rbx, rex\n"
        "v_cmp_gt_u64 vcc, rax2[3:4], rbx4[6:7]\n"
        "v_cmp_gt_u32 vcc, srbx, rex\n"
        "v_cmp_gt_u32 srdx3[2:3], rbx, rex\n"
        "v_cmp_gt_u32 vcc, rbx, srbx\n"
        "v_cmp_gt_u64 vcc, srdx3[3:4], rbx4[6:7]\n"
        "v_cmp_gt_u32 vcc, v72, v41\n"
        "v_cmp_gt_u64 vcc, v[65:66], v[29:30]\n"
        "v_cmp_gt_u64 s[46:47], v[65:66], v[29:30]\n"
        "v_cmp_gt_u32 vcc, v72, s41\n",
        {
            // v_cmp_gt_u32 vcc, rbx, rex
            { 0, "rbx", 0, 1, GCNFIELD_VOP_SRC0, ASMRVU_READ, 1 },
            { 0, "rex", 0, 1, GCNFIELD_VOP_VSRC1, ASMRVU_READ, 1 },
            // v_cmp_gt_u64 vcc, rax[3:4], rbx4[6:7]
            { 4, "rax2", 3, 5, GCNFIELD_VOP_SRC0, ASMRVU_READ, 1 },
            { 4, "rbx4", 6, 8, GCNFIELD_VOP_VSRC1, ASMRVU_READ, 1 },
            // v_cmp_gt_u32 vcc, rbx, rex
            { 8, "srbx", 0, 1, GCNFIELD_VOP_SRC0, ASMRVU_READ, 1 },
            { 8, "rex", 0, 1, GCNFIELD_VOP_VSRC1, ASMRVU_READ, 1 },
            // v_cmp_gt_u32 srdx3[2:3], rbx, rex
            { 12, "srdx3", 2, 4, GCNFIELD_VOP3_SDST0, ASMRVU_WRITE, 1 },
            { 12, "rbx", 0, 1, GCNFIELD_VOP3_SRC0, ASMRVU_READ, 1 },
            { 12, "rex", 0, 1, GCNFIELD_VOP3_SRC1, ASMRVU_READ, 1 },
            // v_cmp_gt_u32 vcc, rbx, srbx
            { 20, "rbx", 0, 1, GCNFIELD_VOP3_SRC0, ASMRVU_READ, 1 },
            { 20, "srbx", 0, 1, GCNFIELD_VOP3_SRC1, ASMRVU_READ, 1 },
            // v_cmp_gt_u64 vcc, srdx3[3:4], rbx4[6:7]
            { 28, "srdx3", 3, 5, GCNFIELD_VOP_SRC0, ASMRVU_READ, 1 },
            { 28, "rbx4", 6, 8, GCNFIELD_VOP_VSRC1, ASMRVU_READ, 1 },
            // v_cmp_gt_u32 vcc, v72, v41
            { 32, nullptr, 256+72, 256+73, GCNFIELD_VOP_SRC0, ASMRVU_READ, 0 },
            { 32, nullptr, 256+41, 256+42, GCNFIELD_VOP_VSRC1, ASMRVU_READ, 0 },
            // v_cmp_gt_u64 vcc, v[65:66], v[29:30]
            { 36, nullptr, 256+65, 256+67, GCNFIELD_VOP_SRC0, ASMRVU_READ, 0 },
            { 36, nullptr, 256+29, 256+31, GCNFIELD_VOP_VSRC1, ASMRVU_READ, 0 },
            // v_cmp_gt_u64 s[46:47], v[65:66], v[29:30]
            { 40, nullptr, 46, 48, GCNFIELD_VOP3_SDST0, ASMRVU_WRITE, 0 },
            { 40, nullptr, 256+65, 256+67, GCNFIELD_VOP3_SRC0, ASMRVU_READ, 0 },
            { 40, nullptr, 256+29, 256+31, GCNFIELD_VOP3_SRC1, ASMRVU_READ, 0 },
            // v_cmp_gt_u32 vcc, v72, s41
            { 48, nullptr, 256+72, 256+73, GCNFIELD_VOP3_SRC0, ASMRVU_READ, 0 },
            { 48, nullptr, 41, 42, GCNFIELD_VOP3_SRC1, ASMRVU_READ, 0 }
        },
        true, ""
    },
    {   /* 11: VOP3 */
        ".regvar rax:v, rbx:v, rcx:v, rex:v\n"
        ".regvar rax2:v:8, rbx4:v:8, rcx4:v:12, rex5:v:10\n"
        ".regvar srex:s, srdx3:s:6, srbx:s\n"
        "v_mad_f32 rex, rax, rbx, rcx\n"
        "v_fma_f64 rex5[3:4], rax2[4:5], rbx4[6:7], rcx4[7:8]\n"
        "v_lshl_b64 rex5[2:3], rbx4[1:2], rcx4[6]\n"
        "v_mad_f32 rex, srbx, rbx, rcx\n"
        "v_mad_f32 rex, rax, srbx, rcx\n"
        "v_mad_f32 rex, rax, rbx, srdx3[4]\n"
        "v_fma_f64 rex5[3:4], rax2[4:5], srdx3[3:4], rcx4[7:8]\n"
        "v_div_scale_f32 rcx, srdx3[3:4], rax, rbx, rex\n"
        /* regusage */
        "v_mad_f32 v54, v12, v21, v73\n"
        "v_fma_f64 v[3:4], v[59:60], v[99:100], v[131:132]\n"
        "v_lshl_b64 v[68:69], v[37:38], v79\n"
        "v_mad_f32 v67, s83, v43, v91\n"
        "v_mad_f32 v67, v83, s43, v91\n"
        "v_mad_f32 v67, v83, v43, s91\n"
        "v_fma_f64 v[153:154], v[73:74], s[83:84], v[17:18]\n"
        "v_div_scale_f32 v184, s[93:94], v53, v14, v89\n",
        {
            // v_mad_f32 rex, rax, rbx, rcx
            { 0, "rex", 0, 1, GCNFIELD_VOP3_VDST, ASMRVU_WRITE, 1 },
            { 0, "rax", 0, 1, GCNFIELD_VOP3_SRC0, ASMRVU_READ, 1 },
            { 0, "rbx", 0, 1, GCNFIELD_VOP3_SRC1, ASMRVU_READ, 1 },
            { 0, "rcx", 0, 1, GCNFIELD_VOP3_SRC2, ASMRVU_READ, 1 },
            // v_fma_f64 rex5[3:4], rax2[4:5], rbx4[6:7], rcx4[7:8]
            { 8, "rex5", 3, 5, GCNFIELD_VOP3_VDST, ASMRVU_WRITE, 1 },
            { 8, "rax2", 4, 6, GCNFIELD_VOP3_SRC0, ASMRVU_READ, 1 },
            { 8, "rbx4", 6, 8, GCNFIELD_VOP3_SRC1, ASMRVU_READ, 1 },
            { 8, "rcx4", 7, 9, GCNFIELD_VOP3_SRC2, ASMRVU_READ, 1 },
            // v_lshl_b64 rex5[2:3], rbx4[1:2], rcx4[6]
            { 16, "rex5", 2, 4, GCNFIELD_VOP3_VDST, ASMRVU_WRITE, 1 },
            { 16, "rbx4", 1, 3, GCNFIELD_VOP3_SRC0, ASMRVU_READ, 1 },
            { 16, "rcx4", 6, 7, GCNFIELD_VOP3_SRC1, ASMRVU_READ, 1 },
            // v_mad_f32 rex, srbx, rbx, rcx
            { 24, "rex", 0, 1, GCNFIELD_VOP3_VDST, ASMRVU_WRITE, 1 },
            { 24, "srbx", 0, 1, GCNFIELD_VOP3_SRC0, ASMRVU_READ, 1 },
            { 24, "rbx", 0, 1, GCNFIELD_VOP3_SRC1, ASMRVU_READ, 1 },
            { 24, "rcx", 0, 1, GCNFIELD_VOP3_SRC2, ASMRVU_READ, 1 },
            // v_mad_f32 rex, rax, srbx, rcx
            { 32, "rex", 0, 1, GCNFIELD_VOP3_VDST, ASMRVU_WRITE, 1 },
            { 32, "rax", 0, 1, GCNFIELD_VOP3_SRC0, ASMRVU_READ, 1 },
            { 32, "srbx", 0, 1, GCNFIELD_VOP3_SRC1, ASMRVU_READ, 1 },
            { 32, "rcx", 0, 1, GCNFIELD_VOP3_SRC2, ASMRVU_READ, 1 },
            // v_mad_f32 rex, rax, rbx, srdx[4]
            { 40, "rex", 0, 1, GCNFIELD_VOP3_VDST, ASMRVU_WRITE, 1 },
            { 40, "rax", 0, 1, GCNFIELD_VOP3_SRC0, ASMRVU_READ, 1 },
            { 40, "rbx", 0, 1, GCNFIELD_VOP3_SRC1, ASMRVU_READ, 1 },
            { 40, "srdx3", 4, 5, GCNFIELD_VOP3_SRC2, ASMRVU_READ, 1 },
            // v_fma_f64 rex5[3:4], rax2[4:5], srdx3[3:4], rcx4[7:8]
            { 48, "rex5", 3, 5, GCNFIELD_VOP3_VDST, ASMRVU_WRITE, 1 },
            { 48, "rax2", 4, 6, GCNFIELD_VOP3_SRC0, ASMRVU_READ, 1 },
            { 48, "srdx3", 3, 5, GCNFIELD_VOP3_SRC1, ASMRVU_READ, 1 },
            { 48, "rcx4", 7, 9, GCNFIELD_VOP3_SRC2, ASMRVU_READ, 1 },
            // v_div_scale_f32 rcx, srdx3[3:4], rax, rbx, rex
            { 56, "rcx", 0, 1, GCNFIELD_VOP3_VDST, ASMRVU_WRITE, 1 },
            { 56, "srdx3", 3, 5, GCNFIELD_VOP3_SDST1, ASMRVU_WRITE, 1 },
            { 56, "rax", 0, 1, GCNFIELD_VOP3_SRC0, ASMRVU_READ, 1 },
            { 56, "rbx", 0, 1, GCNFIELD_VOP3_SRC1, ASMRVU_READ, 1 },
            { 56, "rex", 0, 1, GCNFIELD_VOP3_SRC2, ASMRVU_READ, 1 },
            // v_mad_f32 v54, v12, v21, v73
            { 64, nullptr, 256+54, 256+55, GCNFIELD_VOP3_VDST, ASMRVU_WRITE, 0 },
            { 64, nullptr, 256+12, 256+13, GCNFIELD_VOP3_SRC0, ASMRVU_READ, 0 },
            { 64, nullptr, 256+21, 256+22, GCNFIELD_VOP3_SRC1, ASMRVU_READ, 0 },
            { 64, nullptr, 256+73, 256+74, GCNFIELD_VOP3_SRC2, ASMRVU_READ, 0 },
            // v_fma_f64 v[3:4], v[59:60], v[99:100], v[131:132]
            { 72, nullptr, 256+3, 256+5, GCNFIELD_VOP3_VDST, ASMRVU_WRITE, 0 },
            { 72, nullptr, 256+59, 256+61, GCNFIELD_VOP3_SRC0, ASMRVU_READ, 0 },
            { 72, nullptr, 256+99, 256+101, GCNFIELD_VOP3_SRC1, ASMRVU_READ, 0 },
            { 72, nullptr, 256+131, 256+133, GCNFIELD_VOP3_SRC2, ASMRVU_READ, 0 },
            // v_lshl_b64 v[68:69], rbx4[37:38], v79
            { 80, nullptr, 256+68, 256+70, GCNFIELD_VOP3_VDST, ASMRVU_WRITE, 0 },
            { 80, nullptr, 256+37, 256+39, GCNFIELD_VOP3_SRC0, ASMRVU_READ, 0 },
            { 80, nullptr, 256+79, 256+80, GCNFIELD_VOP3_SRC1, ASMRVU_READ, 0 },
            // v_mad_f32 v67, s83, v43, v91
            { 88, nullptr, 256+67, 256+68, GCNFIELD_VOP3_VDST, ASMRVU_WRITE, 0 },
            { 88, nullptr, 83, 84, GCNFIELD_VOP3_SRC0, ASMRVU_READ, 0 },
            { 88, nullptr, 256+43, 256+44, GCNFIELD_VOP3_SRC1, ASMRVU_READ, 0 },
            { 88, nullptr, 256+91, 256+92, GCNFIELD_VOP3_SRC2, ASMRVU_READ, 0 },
            // v_mad_f32 v67, v83, s43, v91
            { 96, nullptr, 256+67, 256+68, GCNFIELD_VOP3_VDST, ASMRVU_WRITE, 0 },
            { 96, nullptr, 256+83, 256+84, GCNFIELD_VOP3_SRC0, ASMRVU_READ, 0 },
            { 96, nullptr, 43, 44, GCNFIELD_VOP3_SRC1, ASMRVU_READ, 0 },
            { 96, nullptr, 256+91, 256+92, GCNFIELD_VOP3_SRC2, ASMRVU_READ, 0 },
            // v_mad_f32 v67, v83, v43, s91
            { 104, nullptr, 256+67, 256+68, GCNFIELD_VOP3_VDST, ASMRVU_WRITE, 0 },
            { 104, nullptr, 256+83, 256+84, GCNFIELD_VOP3_SRC0, ASMRVU_READ, 0 },
            { 104, nullptr, 256+43, 256+44, GCNFIELD_VOP3_SRC1, ASMRVU_READ, 0 },
            { 104, nullptr, 91, 92, GCNFIELD_VOP3_SRC2, ASMRVU_READ, 0 },
            // v_fma_f64 v[153:154], rax2[73:74], s[83:84], v[17:18]
            { 112, nullptr, 256+153, 256+155, GCNFIELD_VOP3_VDST, ASMRVU_WRITE, 0 },
            { 112, nullptr, 256+73, 256+75, GCNFIELD_VOP3_SRC0, ASMRVU_READ, 0 },
            { 112, nullptr, 83, 85, GCNFIELD_VOP3_SRC1, ASMRVU_READ, 0 },
            { 112, nullptr, 256+17, 256+19, GCNFIELD_VOP3_SRC2, ASMRVU_READ, 0 },
            // v_div_scale_f32 v184, s[93:94], v53, v14, v89
            { 120, nullptr, 256+184, 256+185, GCNFIELD_VOP3_VDST, ASMRVU_WRITE, 0 },
            { 120, nullptr, 93, 95, GCNFIELD_VOP3_SDST1, ASMRVU_WRITE, 0 },
            { 120, nullptr, 256+53, 256+54, GCNFIELD_VOP3_SRC0, ASMRVU_READ, 0 },
            { 120, nullptr, 256+14, 256+15, GCNFIELD_VOP3_SRC1, ASMRVU_READ, 0 },
            { 120, nullptr, 256+89, 256+90, GCNFIELD_VOP3_SRC2, ASMRVU_READ, 0 },
            
        },
        true, ""
    },
    {   /* 12: VOP3 - Fiji */
        ".gpu Fiji\n"
        ".regvar rax:v, rbx:v, rcx:v, rex:v\n"
        ".regvar rax2:v:8, rbx4:v:8, rcx4:v:12, rex5:v:10\n"
        ".regvar srex:s, srdx3:s:6, srbx:s\n"
        "v_mqsad_pk_u16_u8 rax2[1:2], rbx4[3:4], rex5[8], rcx4[4:5]\n"
        "v_mqsad_u32_u8 rax2[1:4], rbx4[3:4], rex5[8], rcx4[4:7]\n"
        "v_interp_p1_f32 rax, rcx, attr39.z vop3\n"
        "v_interp_mov_f32 rax, p20, attr39.z vop3\n"
        "v_interp_p1lv_f16 rax, rbx, attr39.z, srex\n"
        "v_interp_p1lv_f16 rax, rbx, attr39.z, rex\n"
        "v_mqsad_pk_u16_u8 v[51:52], v[74:75], v163, v[82:83]\n"
        "v_mqsad_u32_u8 v[17:20], v[67:68], v117, v[93:96]\n"
        "v_interp_p1lv_f16 v215, v69, attr39.z, s41\n",
        {
            // v_mqsad_pk_u16_u8 rax2[1:2], rbx4[3:4], rex5[8], rcx4[4:5]
            { 0, "rax2", 1, 3, GCNFIELD_VOP3_VDST, ASMRVU_WRITE, 1 },
            { 0, "rbx4", 3, 5, GCNFIELD_VOP3_SRC0, ASMRVU_READ, 1 },
            { 0, "rex5", 8, 9, GCNFIELD_VOP3_SRC1, ASMRVU_READ, 1 },
            { 0, "rcx4", 4, 6, GCNFIELD_VOP3_SRC2, ASMRVU_READ, 1 },
            // v_mqsad_u32_u8 rax2[1:4], rbx4[3:4], rex5[8], rcx4[4:7]
            { 8, "rax2", 1, 5, GCNFIELD_VOP3_VDST, ASMRVU_WRITE, 1 },
            { 8, "rbx4", 3, 5, GCNFIELD_VOP3_SRC0, ASMRVU_READ, 1 },
            { 8, "rex5", 8, 9, GCNFIELD_VOP3_SRC1, ASMRVU_READ, 1 },
            { 8, "rcx4", 4, 8, GCNFIELD_VOP3_SRC2, ASMRVU_READ, 1 },
            // v_interp_p1_f32 rax, rcx, attr39.z vop3
            { 16, "rax", 0, 1, GCNFIELD_VOP3_VDST, ASMRVU_WRITE, 1 },
            { 16, "rcx", 0, 1, GCNFIELD_VOP3_SRC1, ASMRVU_READ, 1 },
            // v_interp_mov_f32 rax, p20, attr39.z vop3
            { 24, "rax", 0, 1, GCNFIELD_VOP3_VDST, ASMRVU_WRITE, 1 },
            // v_interp_p1lv_f16 rax, rbx, attr39.z, srex
            { 32, "rax", 0, 1, GCNFIELD_VOP3_VDST, ASMRVU_WRITE, 1 },
            { 32, "rbx", 0, 1, GCNFIELD_VOP3_SRC1, ASMRVU_READ, 1 },
            { 32, "srex", 0, 1, GCNFIELD_VOP3_SRC2, ASMRVU_READ, 1 },
            // v_interp_p1lv_f16 rax, rbx, attr39.z, rex
            { 40, "rax", 0, 1, GCNFIELD_VOP3_VDST, ASMRVU_WRITE, 1 },
            { 40, "rbx", 0, 1, GCNFIELD_VOP3_SRC1, ASMRVU_READ, 1 },
            { 40, "rex", 0, 1, GCNFIELD_VOP3_SRC2, ASMRVU_READ, 1 },
            // v_mqsad_pk_u16_u8 v[51:52], v[74:75], v163, v[82:83]
            { 48, nullptr, 256+51, 256+53, GCNFIELD_VOP3_VDST, ASMRVU_WRITE, 0 },
            { 48, nullptr, 256+74, 256+76, GCNFIELD_VOP3_SRC0, ASMRVU_READ, 0 },
            { 48, nullptr, 256+163, 256+164, GCNFIELD_VOP3_SRC1, ASMRVU_READ, 0 },
            { 48, nullptr, 256+82, 256+84, GCNFIELD_VOP3_SRC2, ASMRVU_READ, 0 },
            // v_mqsad_u32_u8 v[17:20], v[67:68], v117, v[93:96]
            { 56, nullptr, 256+17, 256+21, GCNFIELD_VOP3_VDST, ASMRVU_WRITE, 0 },
            { 56, nullptr, 256+67, 256+69, GCNFIELD_VOP3_SRC0, ASMRVU_READ, 0 },
            { 56, nullptr, 256+117, 256+118, GCNFIELD_VOP3_SRC1, ASMRVU_READ, 0 },
            { 56, nullptr, 256+93, 256+97, GCNFIELD_VOP3_SRC2, ASMRVU_READ, 0 },
            // v_interp_p1lv_f16 v215, v69, attr39.z, s41
            { 64, nullptr, 256+215, 256+216, GCNFIELD_VOP3_VDST, ASMRVU_WRITE, 0 },
            { 64, nullptr, 256+69, 256+70, GCNFIELD_VOP3_SRC1, ASMRVU_READ, 0 },
            { 64, nullptr, 41, 42, GCNFIELD_VOP3_SRC2, ASMRVU_READ, 0 }
        },
        true, ""
    },
    {   /* 13: VINTRP */
        ".regvar rax:v, rbx:v, rcx:v, rex:v\n"
        ".regvar rax2:v:8, rbx4:v:8, rcx4:v:12, rex5:v:10\n"
        ".regvar srex:s, srdx3:s:6, srbx:s\n"
        "v_interp_p1_f32 rbx, rcx, attr26.w\n"
        "v_interp_mov_f32 rcx4[6], p10, attr26.w\n"
        "v_interp_p1_f32 v85, v24, attr26.w\n"
        "v_interp_mov_f32 v147, p10, attr26.w\n",
        {
            // v_interp_p1_f32 rbx, rcx, attr26.w
            { 0, "rbx", 0, 1, GCNFIELD_VINTRP_VDST, ASMRVU_WRITE, 1 },
            { 0, "rcx", 0, 1, GCNFIELD_VINTRP_VSRC0, ASMRVU_READ, 1 },
            // v_interp_mov_f32 rcx[6], p10, attr26.w
            { 4, "rcx4", 6, 7, GCNFIELD_VINTRP_VDST, ASMRVU_WRITE, 1 },
            // v_interp_p1_f32 v85, v24, attr26.w
            { 8, nullptr, 256+85, 256+86, GCNFIELD_VINTRP_VDST, ASMRVU_WRITE, 0 },
            { 8, nullptr, 256+24, 256+25, GCNFIELD_VINTRP_VSRC0, ASMRVU_READ, 0 },
            // v_interp_mov_f32 v147, p10, attr26.w
            { 12, nullptr, 256+147, 256+148, GCNFIELD_VINTRP_VDST, ASMRVU_WRITE, 0 }
        },
        true, ""
    },
    {   /* 14: DS encoding */
        ".regvar rax:v, rbx:v, rcx:v, rex:v\n"
        ".regvar rax2:v:8, rbx4:v:8, rcx4:v:12, rex5:v:10\n"
        "ds_inc_u32 rbx, rex offset:52583\n"
        "ds_or_rtn_b32 rcx, rbx, rex offset:52583\n"
        "ds_inc_u64 rbx4[1], rex5[6:7] offset:52583\n"
        "ds_or_rtn_b64 rcx4[3:4], rbx4[1], rex5[6:7] offset:52583\n"
        "ds_read_b32 rax, rbx offset:431\n"
        "ds_write_b32 rax, rbx offset:431\n"
        "ds_wrxchg2st64_rtn_b32 rax2[4:5], rax, rbx, rex offset0:103 offset1:205\n"
        "ds_consume rbx4[5] offset:421\n"
        "ds_wrxchg2st64_rtn_b64 rax2[4:7], rax, rbx4[4:5], rex5[8:9] "
                "offset0:103 offset1:205\n"
        "ds_inc_u32 v52, v85 offset:52583\n"
        "ds_or_rtn_b64 v[76:77], v63, v[15:16] offset:52583\n"
        "ds_wrxchg2st64_rtn_b32 v[41:42], v95, v173, v31 offset0:103 offset1:205\n"
        "ds_wrxchg2st64_rtn_b64 v[46:49], v53, v[121:122], v[83:84] "
                "offset0:103 offset1:205\n",
        {
            // ds_inc_u32 rbx, rex offset:52583
            { 0, "rbx", 0, 1, GCNFIELD_DS_ADDR, ASMRVU_READ, 1 },
            { 0, "rex", 0, 1, GCNFIELD_DS_DATA0, ASMRVU_READ, 1 },
            // ds_or_rtn_b32 rcx, rbx, rex offset:52583
            { 8, "rcx", 0, 1, GCNFIELD_DS_VDST, ASMRVU_WRITE, 1 },
            { 8, "rbx", 0, 1, GCNFIELD_DS_ADDR, ASMRVU_READ, 1 },
            { 8, "rex", 0, 1, GCNFIELD_DS_DATA0, ASMRVU_READ, 1 },
            // ds_inc_u64 rbx4[1:2], rex5[6:7] offset:52583
            { 16, "rbx4", 1, 2, GCNFIELD_DS_ADDR, ASMRVU_READ, 1 },
            { 16, "rex5", 6, 8, GCNFIELD_DS_DATA0, ASMRVU_READ, 1 },
            // ds_or_rtn_b64 rcx4[3:4], rbx4[1:2], rex5[6:7] offset:52583
            { 24, "rcx4", 3, 5, GCNFIELD_DS_VDST, ASMRVU_WRITE, 1 },
            { 24, "rbx4", 1, 2, GCNFIELD_DS_ADDR, ASMRVU_READ, 1 },
            { 24, "rex5", 6, 8, GCNFIELD_DS_DATA0, ASMRVU_READ, 1 },
            // ds_read_b32 rax, rbx offset:431
            { 32, "rax", 0, 1, GCNFIELD_DS_VDST, ASMRVU_WRITE, 1 },
            { 32, "rbx", 0, 1, GCNFIELD_DS_ADDR, ASMRVU_READ, 1 },
            // ds_write_b32 rax, rbx offset:431
            { 40, "rax", 0, 1, GCNFIELD_DS_ADDR, ASMRVU_READ, 1 },
            { 40, "rbx", 0, 1, GCNFIELD_DS_DATA0, ASMRVU_READ, 1 },
            // ds_wrxchg2st64_rtn_b32  rax2[4:5], rax, rbx, rex offset0:103 offset1:205
            { 48, "rax2", 4, 6, GCNFIELD_DS_VDST, ASMRVU_WRITE, 1 },
            { 48, "rax", 0, 1, GCNFIELD_DS_ADDR, ASMRVU_READ, 1 },
            { 48, "rbx", 0, 1, GCNFIELD_DS_DATA0, ASMRVU_READ, 1 },
            { 48, "rex", 0, 1, GCNFIELD_DS_DATA1, ASMRVU_READ, 1 },
            // ds_consume rbx4[5] offset:421
            { 56, "rbx4", 5, 6, GCNFIELD_DS_VDST, ASMRVU_WRITE, 1 },
            /* ds_wrxchg2st64_rtn_b64 rax2[4:7], rax, rbx4[4:5], rex5[8:9] 
             * offset0:103 offset1:205 */
            { 64, "rax2", 4, 8, GCNFIELD_DS_VDST, ASMRVU_WRITE, 1 },
            { 64, "rax", 0, 1, GCNFIELD_DS_ADDR, ASMRVU_READ, 1 },
            { 64, "rbx4", 4, 6, GCNFIELD_DS_DATA0, ASMRVU_READ, 1 },
            { 64, "rex5", 8, 10, GCNFIELD_DS_DATA1, ASMRVU_READ, 1 },
            // ds_inc_u32 v52, v85 offset:52583
            { 72, nullptr, 256+52, 256+53, GCNFIELD_DS_ADDR, ASMRVU_READ, 0 },
            { 72, nullptr, 256+85, 256+86, GCNFIELD_DS_DATA0, ASMRVU_READ, 0 },
            // ds_or_rtn_b64 v[76:77], v63, v[15:16] offset:52583
            { 80, nullptr, 256+76, 256+78, GCNFIELD_DS_VDST, ASMRVU_WRITE, 0 },
            { 80, nullptr, 256+63, 256+64, GCNFIELD_DS_ADDR, ASMRVU_READ, 0 },
            { 80, nullptr, 256+15, 256+17, GCNFIELD_DS_DATA0, ASMRVU_READ, 0 },
            // ds_wrxchg2st64_rtn_b32 v[41:42], v95, v173, v31 offset0:103 offset1:205
            { 88, nullptr, 256+41, 256+43, GCNFIELD_DS_VDST, ASMRVU_WRITE, 0 },
            { 88, nullptr, 256+95, 256+96, GCNFIELD_DS_ADDR, ASMRVU_READ, 0 },
            { 88, nullptr, 256+173, 256+174, GCNFIELD_DS_DATA0, ASMRVU_READ, 0 },
            { 88, nullptr, 256+31, 256+32, GCNFIELD_DS_DATA1, ASMRVU_READ, 0 },
            /* ds_wrxchg2st64_rtn_b64 v[46:49], v53, v[121:122], v[83:84] "
                "offset0:103 offset1:205 */
            { 96, nullptr, 256+46, 256+50, GCNFIELD_DS_VDST, ASMRVU_WRITE, 0 },
            { 96, nullptr, 256+53, 256+54, GCNFIELD_DS_ADDR, ASMRVU_READ, 0 },
            { 96, nullptr, 256+121, 256+123, GCNFIELD_DS_DATA0, ASMRVU_READ, 0 },
            { 96, nullptr, 256+83, 256+85, GCNFIELD_DS_DATA1, ASMRVU_READ, 0 }
        },
        true, ""
    },
    {   /* 15: MUBUF/MTBUF encoding */
        ".regvar rax:v, rbx:v, rcx:v, rex:v\n"
        ".regvar rax2:v:8, rbx4:v:8, rcx4:v:12, rex5:v:10\n"
        ".regvar srex:s, srdx3:s:8, srbx:s\n"
        "buffer_load_dword rbx, rex, srdx3[0:3], srbx idxen offset:603\n"
        "buffer_store_dword rbx, rex, srdx3[0:3], srbx idxen offset:603\n"
        "buffer_atomic_add rbx, rex, srdx3[0:3], srbx idxen offset:603\n"
        "buffer_atomic_add rbx, rex, srdx3[0:3], srbx idxen offset:603 glc\n"
        "buffer_atomic_add_x2 rbx4[3:4], rex, srdx3[0:3], srbx idxen offset:603 glc\n"
        "buffer_atomic_cmpswap rcx4[1:2], rex, srdx3[4:7], srbx idxen offset:603 glc\n"
        "buffer_atomic_cmpswap rcx4[1:2], rex, srdx3[0:3], srbx idxen offset:603\n"
        "buffer_atomic_cmpswap_x2 rcx4[3:6], rex, srdx3[4:7], srbx idxen offset:603 glc\n"
        "buffer_load_dwordx4 rex5[5:8], rex, srdx3[0:3], srbx idxen offset:603\n"
        "tbuffer_load_format_xyz rex5[5:7], rex, srdx3[4:7], srbx idxen offset:603\n"
        "buffer_load_format_xyz rex5[5:7], rex, srdx3[4:7], srbx idxen offset:603\n"
        "buffer_wbinvl1\n"
        // regusage
        "buffer_load_dword v45, v21, s[12:15], s52 idxen offset:603\n"
        "buffer_atomic_cmpswap_x2 v[71:74], v41, s[16:19], s43 idxen offset:603 glc\n"
        "buffer_atomic_cmpswap v[64:65], v88, s[12:15], s78 idxen offset:603\n"
        "buffer_atomic_add v59, v13, s[20:23], s74 idxen offset:603 glc\n"
        // various addressing
        "buffer_atomic_add rbx, rex5[3:4], srdx3[0:3], srbx idxen offen offset:603\n"
        "buffer_atomic_add rbx, rex5[4:5], srdx3[4:7], srbx addr64 offset:603\n"
        "buffer_atomic_add rbx, rex, srdx3[0:3], srbx offset:603\n"
        // tfe flag
        "buffer_atomic_add_x2 rbx4[3:5], rex, srdx3[0:3], srbx idxen offset:603 glc tfe\n"
        "buffer_atomic_cmpswap_x2 rcx4[3:7], rex, srdx3[4:7], srbx "
                "idxen offset:603 glc tfe\n"
        "buffer_load_dwordx4 rbx4[1:5], rex, srdx3[0:3], srbx idxen offset:603 tfe\n"
        // regusage (various addressing)
        "buffer_atomic_add v58, v[7:8], s[12:15], s62 idxen offen offset:603\n"
        "buffer_atomic_add v58, v[7], s[12:15], s62 offset:603\n"
        "buffer_atomic_add v58, v[7:8], s[12:15], s62 addr64 offset:603\n"
        // regusage (tfe flag)
        "buffer_atomic_add_x2 v[61:63], v34, s[28:31], s26 idxen offset:603 glc tfe\n"
        "buffer_atomic_cmpswap_x2 v[46:50], v83, s[24:27], s73 idxen offset:603 glc tfe\n"
        "buffer_load_dwordx4 v[11:15], v67, s[20:23], s91 idxen offset:603 tfe\n"
        // other regusage
        "tbuffer_load_format_xyz v[55:57], v76, s[44:47], s61 idxen offset:603\n"
        // have LDS
        "buffer_load_dword rbx, rex, srdx3[0:3], srbx idxen lds offset:603\n"
        "buffer_load_dword v45, v21, s[12:15], s52 idxen lds offset:603\n",
        {
            // buffer_load_dword rbx, rex, srdx3[0:3], srbx idxen offset:603
            { 0, "rbx", 0, 1, GCNFIELD_M_VDATA, ASMRVU_WRITE, 1 },
            { 0, "rex", 0, 1, GCNFIELD_M_VADDR, ASMRVU_READ, 1 },
            { 0, "srdx3", 0, 4, GCNFIELD_M_SRSRC, ASMRVU_READ, 4 },
            { 0, "srbx", 0, 1, GCNFIELD_M_SOFFSET, ASMRVU_READ, 1 },
            // buffer_store_dword rbx, rex, srdx3[0:3], srbx idxen offset:603
            { 8, "rbx", 0, 1, GCNFIELD_M_VDATA, ASMRVU_READ, 1 },
            { 8, "rex", 0, 1, GCNFIELD_M_VADDR, ASMRVU_READ, 1 },
            { 8, "srdx3", 0, 4, GCNFIELD_M_SRSRC, ASMRVU_READ, 4 },
            { 8, "srbx", 0, 1, GCNFIELD_M_SOFFSET, ASMRVU_READ, 1 },
            // buffer_atomic_add rbx, rex, srdx3[0:3], srbx idxen offset:603
            { 16, "rbx", 0, 1, GCNFIELD_M_VDATA, ASMRVU_READ, 1 },
            { 16, "rex", 0, 1, GCNFIELD_M_VADDR, ASMRVU_READ, 1 },
            { 16, "srdx3", 0, 4, GCNFIELD_M_SRSRC, ASMRVU_READ, 4 },
            { 16, "srbx", 0, 1, GCNFIELD_M_SOFFSET, ASMRVU_READ, 1 },
            // buffer_atomic_add rbx, rex, srdx3[0:3], srbx idxen offset:603 glc
            { 24, "rbx", 0, 1, GCNFIELD_M_VDATA, ASMRVU_READ|ASMRVU_WRITE, 1 },
            { 24, "rex", 0, 1, GCNFIELD_M_VADDR, ASMRVU_READ, 1 },
            { 24, "srdx3", 0, 4, GCNFIELD_M_SRSRC, ASMRVU_READ, 4 },
            { 24, "srbx", 0, 1, GCNFIELD_M_SOFFSET, ASMRVU_READ, 1 },
            // buffer_atomic_add_x2 rbx4[3:4], rex, srdx3[0:3], srbx idxen offset:603 glc
            { 32, "rbx4", 3, 5, GCNFIELD_M_VDATA, ASMRVU_READ|ASMRVU_WRITE, 1 },
            { 32, "rex", 0, 1, GCNFIELD_M_VADDR, ASMRVU_READ, 1 },
            { 32, "srdx3", 0, 4, GCNFIELD_M_SRSRC, ASMRVU_READ, 4 },
            { 32, "srbx", 0, 1, GCNFIELD_M_SOFFSET, ASMRVU_READ, 1 },
            // buffer_atomic_cmpswap rcx4[1:2], rex, srdx3[0:3], srbx idxen offset:603 glc
            { 40, "rcx4", 1, 2, GCNFIELD_M_VDATA, ASMRVU_READ|ASMRVU_WRITE, 1 },
            { 40, "rex", 0, 1, GCNFIELD_M_VADDR, ASMRVU_READ, 1 },
            { 40, "srdx3", 4, 8, GCNFIELD_M_SRSRC, ASMRVU_READ, 4 },
            { 40, "srbx", 0, 1, GCNFIELD_M_SOFFSET, ASMRVU_READ, 1 },
            { 40, "rcx4", 2, 3, GCNFIELD_M_VDATAH, ASMRVU_READ, 1 },
            // buffer_atomic_cmpswap rcx4[1:2], rex, srdx3[0:3], srbx idxen offset:603
            { 48, "rcx4", 1, 3, GCNFIELD_M_VDATA, ASMRVU_READ, 1 },
            { 48, "rex", 0, 1, GCNFIELD_M_VADDR, ASMRVU_READ, 1 },
            { 48, "srdx3", 0, 4, GCNFIELD_M_SRSRC, ASMRVU_READ, 4 },
            { 48, "srbx", 0, 1, GCNFIELD_M_SOFFSET, ASMRVU_READ, 1 },
            /* buffer_atomic_cmpswap_x2 rcx4[3:6], rex, srdx3[0:3],
                srbx idxen offset:603 glc */
            { 56, "rcx4", 3, 5, GCNFIELD_M_VDATA, ASMRVU_READ|ASMRVU_WRITE, 1 },
            { 56, "rex", 0, 1, GCNFIELD_M_VADDR, ASMRVU_READ, 1 },
            { 56, "srdx3", 4, 8, GCNFIELD_M_SRSRC, ASMRVU_READ, 4 },
            { 56, "srbx", 0, 1, GCNFIELD_M_SOFFSET, ASMRVU_READ, 1 },
            { 56, "rcx4", 5, 7, GCNFIELD_M_VDATAH, ASMRVU_READ, 1 },
            // buffer_load_dwordx4 rex5[5:8], rex, srdx3[0:3], srbx idxen offset:603
            { 64, "rex5", 5, 9, GCNFIELD_M_VDATA, ASMRVU_WRITE, 1 },
            { 64, "rex", 0, 1, GCNFIELD_M_VADDR, ASMRVU_READ, 1 },
            { 64, "srdx3", 0, 4, GCNFIELD_M_SRSRC, ASMRVU_READ, 4 },
            { 64, "srbx", 0, 1, GCNFIELD_M_SOFFSET, ASMRVU_READ, 1 },
            // tbuffer_load_format_xyz rex5[5:7], rex, srdx3[0:3], srbx idxen offset:603
            { 72, "rex5", 5, 8, GCNFIELD_M_VDATA, ASMRVU_WRITE, 1 },
            { 72, "rex", 0, 1, GCNFIELD_M_VADDR, ASMRVU_READ, 1 },
            { 72, "srdx3", 4, 8, GCNFIELD_M_SRSRC, ASMRVU_READ, 4 },
            { 72, "srbx", 0, 1, GCNFIELD_M_SOFFSET, ASMRVU_READ, 1 },
            // buffer_load_format_xyz rex5[5:7], rex, srdx3[0:3], srbx idxen offset:603
            { 80, "rex5", 5, 8, GCNFIELD_M_VDATA, ASMRVU_WRITE, 1 },
            { 80, "rex", 0, 1, GCNFIELD_M_VADDR, ASMRVU_READ, 1 },
            { 80, "srdx3", 4, 8, GCNFIELD_M_SRSRC, ASMRVU_READ, 4 },
            { 80, "srbx", 0, 1, GCNFIELD_M_SOFFSET, ASMRVU_READ, 1 },
            // buffer_load_dword v45, v21, s[12:15], s52 idxen offset:603
            { 96, nullptr, 256+45, 256+46, GCNFIELD_M_VDATA, ASMRVU_WRITE, 0 },
            { 96, nullptr, 256+21, 256+22, GCNFIELD_M_VADDR, ASMRVU_READ, 0 },
            { 96, nullptr, 12, 16, GCNFIELD_M_SRSRC, ASMRVU_READ, 0 },
            { 96, nullptr, 52, 53, GCNFIELD_M_SOFFSET, ASMRVU_READ, 0 },
            // buffer_atomic_cmpswap_x2 v[71:74], v41, s[16:19], s43 idxen offset:603 glc
            { 104, nullptr, 256+71, 256+73, GCNFIELD_M_VDATA, ASMRVU_READ|ASMRVU_WRITE, 0 },
            { 104, nullptr, 256+41, 256+42, GCNFIELD_M_VADDR, ASMRVU_READ, 0 },
            { 104, nullptr, 16, 20, GCNFIELD_M_SRSRC, ASMRVU_READ, 0 },
            { 104, nullptr, 43, 44, GCNFIELD_M_SOFFSET, ASMRVU_READ, 0 },
            { 104, nullptr, 256+73, 256+75, GCNFIELD_M_VDATAH, ASMRVU_READ, 0 },
            // buffer_atomic_cmpswap v[64:65], v88, s[12:15], s78 idxen offset:603
            { 112, nullptr, 256+64, 256+66, GCNFIELD_M_VDATA, ASMRVU_READ, 0 },
            { 112, nullptr, 256+88, 256+89, GCNFIELD_M_VADDR, ASMRVU_READ, 0 },
            { 112, nullptr, 12, 16, GCNFIELD_M_SRSRC, ASMRVU_READ, 0 },
            { 112, nullptr, 78, 79, GCNFIELD_M_SOFFSET, ASMRVU_READ, 0 },
            // buffer_atomic_add v59, v13, s[20:23], s74 idxen offset:603 glc
            { 120, nullptr, 256+59, 256+60, GCNFIELD_M_VDATA, ASMRVU_READ|ASMRVU_WRITE, 0 },
            { 120, nullptr, 256+13, 256+14, GCNFIELD_M_VADDR, ASMRVU_READ, 0 },
            { 120, nullptr, 20, 24, GCNFIELD_M_SRSRC, ASMRVU_READ, 0 },
            { 120, nullptr, 74, 75, GCNFIELD_M_SOFFSET, ASMRVU_READ, 0 },
            // buffer_atomic_add rbx, rex5[3:4], srdx3[0:3], srbx idxen offen offset:603
            { 128, "rbx", 0, 1, GCNFIELD_M_VDATA, ASMRVU_READ, 1 },
            { 128, "rex5", 3, 5, GCNFIELD_M_VADDR, ASMRVU_READ, 1 },
            { 128, "srdx3", 0, 4, GCNFIELD_M_SRSRC, ASMRVU_READ, 4 },
            { 128, "srbx", 0, 1, GCNFIELD_M_SOFFSET, ASMRVU_READ, 1 },
            // buffer_atomic_add rbx, rex5[4:5], srdx3[4:7], srbx addr64 offset:603
            { 136, "rbx", 0, 1, GCNFIELD_M_VDATA, ASMRVU_READ, 1 },
            { 136, "rex5", 4, 6, GCNFIELD_M_VADDR, ASMRVU_READ, 1 },
            { 136, "srdx3", 4, 8, GCNFIELD_M_SRSRC, ASMRVU_READ, 4 },
            { 136, "srbx", 0, 1, GCNFIELD_M_SOFFSET, ASMRVU_READ, 1 },
            // buffer_atomic_add rbx, rex, srdx3[0:3], srbx offset:603
            { 144, "rbx", 0, 1, GCNFIELD_M_VDATA, ASMRVU_READ, 1 },
            { 144, "srdx3", 0, 4, GCNFIELD_M_SRSRC, ASMRVU_READ, 4 },
            { 144, "srbx", 0, 1, GCNFIELD_M_SOFFSET, ASMRVU_READ, 1 },
            /* buffer_atomic_add_x2 rbx4[3:4], rex, srdx3[0:3], srbx
             *      idxen offset:603 glc tfe */
            { 152, "rbx4", 3, 6, GCNFIELD_M_VDATA, ASMRVU_READ|ASMRVU_WRITE, 1 },
            { 152, "rex", 0, 1, GCNFIELD_M_VADDR, ASMRVU_READ, 1 },
            { 152, "srdx3", 0, 4, GCNFIELD_M_SRSRC, ASMRVU_READ, 4 },
            { 152, "srbx", 0, 1, GCNFIELD_M_SOFFSET, ASMRVU_READ, 1 },
            /* buffer_atomic_cmpswap_x2 rcx4[3:6], rex, srdx3[4:7], srbx
                idxen offset:603 glc tfe */
            { 160, "rcx4", 3, 5, GCNFIELD_M_VDATA, ASMRVU_READ|ASMRVU_WRITE, 1 },
            { 160, "rex", 0, 1, GCNFIELD_M_VADDR, ASMRVU_READ, 1 },
            { 160, "srdx3", 4, 8, GCNFIELD_M_SRSRC, ASMRVU_READ, 4 },
            { 160, "srbx", 0, 1, GCNFIELD_M_SOFFSET, ASMRVU_READ, 1 },
            { 160, "rcx4", 5, 7, GCNFIELD_M_VDATAH, ASMRVU_READ, 1 },
            { 160, "rcx4", 7, 8, GCNFIELD_M_VDATALAST, ASMRVU_READ|ASMRVU_WRITE, 1 },
            // buffer_load_dwordx4 rbx4[1:5], rex, srdx3[0:3], srbx idxen offset:603 tfe
            { 168, "rbx4", 1, 5, GCNFIELD_M_VDATA, ASMRVU_WRITE, 1 },
            { 168, "rex", 0, 1, GCNFIELD_M_VADDR, ASMRVU_READ, 1 },
            { 168, "srdx3", 0, 4, GCNFIELD_M_SRSRC, ASMRVU_READ, 4 },
            { 168, "srbx", 0, 1, GCNFIELD_M_SOFFSET, ASMRVU_READ, 1 },
            { 168, "rbx4", 5, 6, GCNFIELD_M_VDATALAST, ASMRVU_READ|ASMRVU_WRITE, 1 },
            // buffer_atomic_add v58, v[7:8], s[12:15], s62 idxen offen offset:603
            { 176, nullptr, 256+58, 256+59, GCNFIELD_M_VDATA, ASMRVU_READ, 0 },
            { 176, nullptr, 256+7, 256+9, GCNFIELD_M_VADDR, ASMRVU_READ, 0 },
            { 176, nullptr, 12, 16, GCNFIELD_M_SRSRC, ASMRVU_READ, 0 },
            { 176, nullptr, 62, 63, GCNFIELD_M_SOFFSET, ASMRVU_READ, 0 },
            // buffer_atomic_add v58, v[7:8], s[12:15], s62 offset:603
            { 184, nullptr, 256+58, 256+59, GCNFIELD_M_VDATA, ASMRVU_READ, 0 },
            { 184, nullptr, 12, 16, GCNFIELD_M_SRSRC, ASMRVU_READ, 0 },
            { 184, nullptr, 62, 63, GCNFIELD_M_SOFFSET, ASMRVU_READ, 0 },
            // buffer_atomic_add v58, v[7:8], s[12:15], s62 addr64 offset:603
            { 192, nullptr, 256+58, 256+59, GCNFIELD_M_VDATA, ASMRVU_READ, 0 },
            { 192, nullptr, 256+7, 256+9, GCNFIELD_M_VADDR, ASMRVU_READ, 0 },
            { 192, nullptr, 12, 16, GCNFIELD_M_SRSRC, ASMRVU_READ, 0 },
            { 192, nullptr, 62, 63, GCNFIELD_M_SOFFSET, ASMRVU_READ, 0 },
            // buffer_atomic_add_x2 v[61:63], v34, s[28:31], s26 idxen offset:603 glc tfe
            { 200, nullptr, 256+61, 256+64, GCNFIELD_M_VDATA, ASMRVU_READ|ASMRVU_WRITE, 0 },
            { 200, nullptr, 256+34, 256+35, GCNFIELD_M_VADDR, ASMRVU_READ, 0 },
            { 200, nullptr, 28, 32, GCNFIELD_M_SRSRC, ASMRVU_READ, 0 },
            { 200, nullptr, 26, 27, GCNFIELD_M_SOFFSET, ASMRVU_READ, 0 },
            /* buffer_atomic_cmpswap_x2 v[46:50], v83, s[24:27], s73 
                 idxen offset:603 glc tfe */
            { 208, nullptr, 256+46, 256+48, GCNFIELD_M_VDATA, ASMRVU_READ|ASMRVU_WRITE, 0 },
            { 208, nullptr, 256+83, 256+84, GCNFIELD_M_VADDR, ASMRVU_READ, 0 },
            { 208, nullptr, 24, 28, GCNFIELD_M_SRSRC, ASMRVU_READ, 0 },
            { 208, nullptr, 73, 74, GCNFIELD_M_SOFFSET, ASMRVU_READ, 0 },
            { 208, nullptr, 256+48, 256+50, GCNFIELD_M_VDATAH, ASMRVU_READ, 0 },
            { 208, nullptr, 256+50, 256+51, GCNFIELD_M_VDATALAST,
                ASMRVU_READ|ASMRVU_WRITE, 0 },
            // buffer_load_dwordx4 v[11:15], v67, s[20:23], s91 idxen offset:603 tfe
            { 216, nullptr, 256+11, 256+15, GCNFIELD_M_VDATA, ASMRVU_WRITE, 0 },
            { 216, nullptr, 256+67, 256+68, GCNFIELD_M_VADDR, ASMRVU_READ, 0 },
            { 216, nullptr, 20, 24, GCNFIELD_M_SRSRC, ASMRVU_READ, 0 },
            { 216, nullptr, 91, 92, GCNFIELD_M_SOFFSET, ASMRVU_READ, 0 },
            { 216, nullptr, 256+15, 256+16, GCNFIELD_M_VDATALAST,
                ASMRVU_READ|ASMRVU_WRITE, 0 },
            // tbuffer_load_format_xyz v[55:57], v76, s[44:47], s61 idxen offset:603
            { 224, nullptr, 256+55, 256+58, GCNFIELD_M_VDATA, ASMRVU_WRITE, 0 },
            { 224, nullptr, 256+76, 256+77, GCNFIELD_M_VADDR, ASMRVU_READ, 0 },
            { 224, nullptr, 44, 48, GCNFIELD_M_SRSRC, ASMRVU_READ, 0 },
            { 224, nullptr, 61, 62, GCNFIELD_M_SOFFSET, ASMRVU_READ, 0 },
            // buffer_load_dword rbx, rex, srdx3[0:3], srbx idxen lds offset:603
            { 232, "rex", 0, 1, GCNFIELD_M_VADDR, ASMRVU_READ, 1 },
            { 232, "srdx3", 0, 4, GCNFIELD_M_SRSRC, ASMRVU_READ, 4 },
            { 232, "srbx", 0, 1, GCNFIELD_M_SOFFSET, ASMRVU_READ, 1 },
            // buffer_load_dword v45, v21, s[12:15], s52 idxen lds offset:603
            { 240, nullptr, 256+21, 256+22, GCNFIELD_M_VADDR, ASMRVU_READ, 0 },
            { 240, nullptr, 12, 16, GCNFIELD_M_SRSRC, ASMRVU_READ, 0 },
            { 240, nullptr, 52, 53, GCNFIELD_M_SOFFSET, ASMRVU_READ, 0 },
        },
        true, ""
    },
    {   /* 16: MIMG encoding */
        ".regvar rax:v, rbx:v, rcx:v, rex:v\n"
        ".regvar rax2:v:10, rbx4:v:8, rcx4:v:12, rex5:v:10\n"
        ".regvar srex:s, srdx3:s:8, srbx:s, srcx5:s:8\n"
        "image_load rax, rcx4[1:4], srdx3[0:3] dmask:1 unorm r128\n"
        "image_load rax, rcx4[1:4], srdx3[0:7] dmask:1 unorm\n"
        "image_load rax2[1:3], rcx4[1:4], srdx3[0:7] dmask:13 unorm\n"
        "image_sample_c_cd_cl_o rax2[1:3], rcx4[1:6], srdx3[0:7], srcx5[4:7] "
                    "dmask:13 unorm\n"
        "image_store rax2[5:6], rcx4[1:4], srdx3[0:3] dmask:5 unorm r128\n"
        "image_atomic_add rax2[5:6], rcx4[1:4], srdx3[0:3] dmask:5 unorm r128\n"
        "image_atomic_add rax2[5:6], rcx4[1:4], srdx3[0:3] dmask:5 unorm glc r128\n"
        "image_atomic_cmpswap rax2[5:6], rcx4[1:4], srdx3[0:3] dmask:5 unorm glc r128\n"
        "image_atomic_cmpswap rax2[4:7], rcx4[1:4], srdx3[0:7] dmask:15 unorm glc\n"
        "image_atomic_cmpswap rax2[4:7], rcx4[1:4], srdx3[0:7] dmask:15 unorm\n"
        "image_gather4_b_cl rax2[1:4], rcx4[1:4], srdx3[0:7], srcx5[4:7] dmask:13 unorm\n"
        // tfe
        "image_load rax2[1:4], rcx4[1:4], srdx3[0:7] dmask:13 unorm tfe\n"
        "image_atomic_cmpswap rax2[4:8], rcx4[1:4], srdx3[0:7] dmask:15 unorm glc tfe\n"
        "image_atomic_add rax2[5:7], rcx4[1:4], srdx3[0:3] dmask:5 unorm glc r128 tfe\n"
        "image_atomic_add rax2[5:7], rcx4[1:4], srdx3[0:3] dmask:5 unorm r128 tfe\n"
        // regusage
        "image_load v76, v[121:124], s[24:27] dmask:1 unorm r128\n"
        "image_sample_c_cd_cl_o v[73:75], v[66:71], s[44:51], s[36:39] dmask:13 unorm\n"
        "image_atomic_add v[15:16], v[57:60], s[52:55] dmask:5 unorm glc r128\n"
        "image_atomic_add v[15:16], v[57:60], s[52:55] dmask:5 unorm r128\n"
        "image_atomic_cmpswap v[5:8], v[11:14], s[8:11] dmask:15 unorm glc r128\n"
        // tfe
        "image_load v[75:78], v[92:95], s[20:27] dmask:13 unorm tfe\n"
        "image_atomic_cmpswap v[62:66], v[35:38], s[20:27] dmask:15 unorm glc tfe\n"
        "image_atomic_add v[87:89], v[24:27], s[28:31] dmask:5 unorm glc r128 tfe\n"
        "image_atomic_add v[87:89], v[24:27], s[28:31] dmask:5 unorm r128 tfe\n"
        // resinfo
        "image_get_resinfo rax, rcx4[1:4], srdx3[0:3] dmask:1 unorm r128\n"
        "image_get_lod rax, rcx4[1:4], srdx3[0:3], srcx5[4:7] dmask:1 unorm r128\n"
        // fcmpswap
        "image_atomic_fcmpswap rax2[5:6], rcx4[1:4], srdx3[0:3] dmask:5 unorm glc r128\n",
        {
            // image_load rax, rcx4[1:4], srdx3[0:3] dmask:1 unorm r128
            { 0, "rax", 0, 1, GCNFIELD_M_VDATA, ASMRVU_WRITE, 1 },
            { 0, "rcx4", 1, 5, GCNFIELD_M_VADDR, ASMRVU_READ, 1 },
            { 0, "srdx3", 0, 4, GCNFIELD_M_SRSRC, ASMRVU_READ, 4 },
            // image_load rax, rcx4[1:4], srdx3[0:3] dmask:1 unorm
            { 8, "rax", 0, 1, GCNFIELD_M_VDATA, ASMRVU_WRITE, 1 },
            { 8, "rcx4", 1, 5, GCNFIELD_M_VADDR, ASMRVU_READ, 1 },
            { 8, "srdx3", 0, 8, GCNFIELD_M_SRSRC, ASMRVU_READ, 4 },
            // image_load rax2[1:3], rcx4[1:4], srdx3[0:7] dmask:13 unorm
            { 16, "rax2", 1, 4, GCNFIELD_M_VDATA, ASMRVU_WRITE, 1 },
            { 16, "rcx4", 1, 5, GCNFIELD_M_VADDR, ASMRVU_READ, 1 },
            { 16, "srdx3", 0, 8, GCNFIELD_M_SRSRC, ASMRVU_READ, 4 },
            // image_sample_c_cd_cl_o rax2[1:3], rcx4[1:6], srdx3[0:7] dmask:13 unorm
            { 24, "rax2", 1, 4, GCNFIELD_M_VDATA, ASMRVU_WRITE, 1 },
            { 24, "rcx4", 1, 7, GCNFIELD_M_VADDR, ASMRVU_READ, 1 },
            { 24, "srdx3", 0, 8, GCNFIELD_M_SRSRC, ASMRVU_READ, 4 },
            { 24, "srcx5", 4, 8, GCNFIELD_MIMG_SSAMP, ASMRVU_READ, 4 },
            // image_store rax2[5:6], rcx4[1:4], srdx3[0:3] dmask:1 unorm r128
            { 32, "rax2", 5, 7, GCNFIELD_M_VDATA, ASMRVU_READ, 1 },
            { 32, "rcx4", 1, 5, GCNFIELD_M_VADDR, ASMRVU_READ, 1 },
            { 32, "srdx3", 0, 4, GCNFIELD_M_SRSRC, ASMRVU_READ, 4 },
            // image_atomic_add rax2[5:6], rcx4[1:4], srdx3[0:3] dmask:5 unorm r128
            { 40, "rax2", 5, 7, GCNFIELD_M_VDATA, ASMRVU_READ, 1 },
            { 40, "rcx4", 1, 5, GCNFIELD_M_VADDR, ASMRVU_READ, 1 },
            { 40, "srdx3", 0, 4, GCNFIELD_M_SRSRC, ASMRVU_READ, 4 },
            // image_atomic_add rax2[5:6], rcx4[1:4], srdx3[0:3] dmask:5 unorm glc r128
            { 48, "rax2", 5, 7, GCNFIELD_M_VDATA, ASMRVU_READ|ASMRVU_WRITE, 1 },
            { 48, "rcx4", 1, 5, GCNFIELD_M_VADDR, ASMRVU_READ, 1 },
            { 48, "srdx3", 0, 4, GCNFIELD_M_SRSRC, ASMRVU_READ, 4 },
            // image_atomic_cmpswap rax2[5:6], rcx4[1:4], srdx3[0:3] dmask:5 unorm glc r128
            { 56, "rax2", 5, 6, GCNFIELD_M_VDATA, ASMRVU_READ|ASMRVU_WRITE, 1 },
            { 56, "rcx4", 1, 5, GCNFIELD_M_VADDR, ASMRVU_READ, 1 },
            { 56, "srdx3", 0, 4, GCNFIELD_M_SRSRC, ASMRVU_READ, 4 },
            { 56, "rax2", 6, 7, GCNFIELD_M_VDATAH, ASMRVU_READ, 1 },
            // image_atomic_cmpswap rax2[4:7], rcx4[1:4], srdx3[0:7] dmask:15 unorm glc
            { 64, "rax2", 4, 6, GCNFIELD_M_VDATA, ASMRVU_READ|ASMRVU_WRITE, 1 },
            { 64, "rcx4", 1, 5, GCNFIELD_M_VADDR, ASMRVU_READ, 1 },
            { 64, "srdx3", 0, 8, GCNFIELD_M_SRSRC, ASMRVU_READ, 4 },
            { 64, "rax2", 6, 8, GCNFIELD_M_VDATAH, ASMRVU_READ, 1 },
            // image_atomic_cmpswap rax2[4:7], rcx4[1:4], srdx3[0:7] dmask:15 unorm
            { 72, "rax2", 4, 8, GCNFIELD_M_VDATA, ASMRVU_READ, 1 },
            { 72, "rcx4", 1, 5, GCNFIELD_M_VADDR, ASMRVU_READ, 1 },
            { 72, "srdx3", 0, 8, GCNFIELD_M_SRSRC, ASMRVU_READ, 4 },
            /* image_gather4_b_cl rax2[1:4], rcx4[1:4], srdx3[0:7],
             * srcx5[4:7] dmask:13 unorm */
            { 80, "rax2", 1, 5, GCNFIELD_M_VDATA, ASMRVU_WRITE, 1 },
            { 80, "rcx4", 1, 5, GCNFIELD_M_VADDR, ASMRVU_READ, 1 },
            { 80, "srdx3", 0, 8, GCNFIELD_M_SRSRC, ASMRVU_READ, 4 },
            { 80, "srcx5", 4, 8, GCNFIELD_MIMG_SSAMP, ASMRVU_READ, 4 },
            // image_load rax2[1:4], rcx4[1:4], srdx3[0:7] dmask:13 unorm tfe
            { 88, "rax2", 1, 4, GCNFIELD_M_VDATA, ASMRVU_WRITE, 1 },
            { 88, "rcx4", 1, 5, GCNFIELD_M_VADDR, ASMRVU_READ, 1 },
            { 88, "srdx3", 0, 8, GCNFIELD_M_SRSRC, ASMRVU_READ, 4 },
            { 88, "rax2", 4, 5, GCNFIELD_M_VDATALAST, ASMRVU_READ|ASMRVU_WRITE, 1 },
            // image_atomic_cmpswap rax2[4:8], rcx4[1:4], srdx3[0:7] dmask:15 unorm glc tfe
            { 96, "rax2", 4, 6, GCNFIELD_M_VDATA, ASMRVU_READ|ASMRVU_WRITE, 1 },
            { 96, "rcx4", 1, 5, GCNFIELD_M_VADDR, ASMRVU_READ, 1 },
            { 96, "srdx3", 0, 8, GCNFIELD_M_SRSRC, ASMRVU_READ, 4 },
            { 96, "rax2", 6, 8, GCNFIELD_M_VDATAH, ASMRVU_READ, 1 },
            { 96, "rax2", 8, 9, GCNFIELD_M_VDATALAST, ASMRVU_READ|ASMRVU_WRITE, 1 },
            // image_atomic_add rax2[5:7], rcx4[1:4], srdx3[0:3] dmask:5 unorm glc r128 tfe
            { 104, "rax2", 5, 8, GCNFIELD_M_VDATA, ASMRVU_READ|ASMRVU_WRITE, 1 },
            { 104, "rcx4", 1, 5, GCNFIELD_M_VADDR, ASMRVU_READ, 1 },
            { 104, "srdx3", 0, 4, GCNFIELD_M_SRSRC, ASMRVU_READ, 4 },
            // image_atomic_add rax2[5:7], rcx4[1:4], srdx3[0:3] dmask:5 unorm r128 tfe
            { 112, "rax2", 5, 7, GCNFIELD_M_VDATA, ASMRVU_READ, 1 },
            { 112, "rcx4", 1, 5, GCNFIELD_M_VADDR, ASMRVU_READ, 1 },
            { 112, "srdx3", 0, 4, GCNFIELD_M_SRSRC, ASMRVU_READ, 4 },
            { 112, "rax2", 7, 8, GCNFIELD_M_VDATALAST, ASMRVU_READ|ASMRVU_WRITE, 1 },
            // image_load v76, v[121:124], s[24:27] dmask:1 unorm r128
            { 120, nullptr, 256+76, 256+77, GCNFIELD_M_VDATA, ASMRVU_WRITE, 0 },
            { 120, nullptr, 256+121, 256+125, GCNFIELD_M_VADDR, ASMRVU_READ, 0 },
            { 120, nullptr, 24, 28, GCNFIELD_M_SRSRC, ASMRVU_READ, 0 },
            // image_sample_c_cd_cl_o v[73:75], v[66:71], s[44:51], s[36:39] dmask:13 unorm
            { 128, nullptr, 256+73, 256+76, GCNFIELD_M_VDATA, ASMRVU_WRITE, 0 },
            { 128, nullptr, 256+66, 256+72, GCNFIELD_M_VADDR, ASMRVU_READ, 0 },
            { 128, nullptr, 44, 52, GCNFIELD_M_SRSRC, ASMRVU_READ, 0 },
            { 128, nullptr, 36, 40, GCNFIELD_MIMG_SSAMP, ASMRVU_READ, 0 },
            // image_atomic_add v[15:16], v[57:60], s[52:55] dmask:5 unorm glc r128
            { 136, nullptr, 256+15, 256+17, GCNFIELD_M_VDATA, ASMRVU_READ|ASMRVU_WRITE, 0 },
            { 136, nullptr, 256+57, 256+61, GCNFIELD_M_VADDR, ASMRVU_READ, 0 },
            { 136, nullptr, 52, 56, GCNFIELD_M_SRSRC, ASMRVU_READ, 0 },
            // image_atomic_add v[15:16], v[57:60], s[52:55] dmask:5 unorm r128
            { 144, nullptr, 256+15, 256+17, GCNFIELD_M_VDATA, ASMRVU_READ, 0 },
            { 144, nullptr, 256+57, 256+61, GCNFIELD_M_VADDR, ASMRVU_READ, 0 },
            { 144, nullptr, 52, 56, GCNFIELD_M_SRSRC, ASMRVU_READ, 0 },
            // image_atomic_cmpswap v[5:8], v[11:14], s[8:11] dmask:15 unorm glc r128
            { 152, nullptr, 256+5, 256+7, GCNFIELD_M_VDATA, ASMRVU_READ|ASMRVU_WRITE, 0 },
            { 152, nullptr, 256+11, 256+15, GCNFIELD_M_VADDR, ASMRVU_READ, 0 },
            { 152, nullptr, 8, 12, GCNFIELD_M_SRSRC, ASMRVU_READ, 0 },
            { 152, nullptr, 256+7, 256+9, GCNFIELD_M_VDATAH, ASMRVU_READ, 0 },
            // image_load v[75:78], v[92:95], s[20:27] dmask:13 unorm tfe
            { 160, nullptr, 256+75, 256+78, GCNFIELD_M_VDATA, ASMRVU_WRITE, 0 },
            { 160, nullptr, 256+92, 256+96, GCNFIELD_M_VADDR, ASMRVU_READ, 0 },
            { 160, nullptr, 20, 28, GCNFIELD_M_SRSRC, ASMRVU_READ, 0 },
            { 160, nullptr, 256+78, 256+79, GCNFIELD_M_VDATALAST,
                    ASMRVU_READ|ASMRVU_WRITE, 0 },
            // image_atomic_cmpswap v[62:66], v[35:38], s[20:27] dmask:15 unorm glc tfe
            { 168, nullptr, 256+62, 256+64, GCNFIELD_M_VDATA, ASMRVU_READ|ASMRVU_WRITE, 0 },
            { 168, nullptr, 256+35, 256+39, GCNFIELD_M_VADDR, ASMRVU_READ, 0 },
            { 168, nullptr, 20, 28, GCNFIELD_M_SRSRC, ASMRVU_READ, 0 },
            { 168, nullptr, 256+64, 256+66, GCNFIELD_M_VDATAH, ASMRVU_READ, 0 },
            { 168, nullptr, 256+66, 256+67, GCNFIELD_M_VDATALAST,
                    ASMRVU_READ|ASMRVU_WRITE, 0 },
            // image_atomic_add v[87:89], v[24:27], s[28:31] dmask:5 unorm glc r128 tfe
            { 176, nullptr, 256+87, 256+90, GCNFIELD_M_VDATA, ASMRVU_READ|ASMRVU_WRITE, 0 },
            { 176, nullptr, 256+24, 256+28, GCNFIELD_M_VADDR, ASMRVU_READ, 0 },
            { 176, nullptr, 28, 32, GCNFIELD_M_SRSRC, ASMRVU_READ, 0 },
            // image_atomic_add v[87:89], v[24:27], s[28:31] dmask:5 unorm r128 tfe
            { 184, nullptr, 256+87, 256+89, GCNFIELD_M_VDATA, ASMRVU_READ, 0 },
            { 184, nullptr, 256+24, 256+28, GCNFIELD_M_VADDR, ASMRVU_READ, 0 },
            { 184, nullptr, 28, 32, GCNFIELD_M_SRSRC, ASMRVU_READ, 0 },
            { 184, nullptr, 256+89, 256+90, GCNFIELD_M_VDATALAST,
                    ASMRVU_READ|ASMRVU_WRITE, 0 },
            // image_get_resinfo rax, rcx4[1:4], srdx3[0:3] dmask:1 unorm r128
            { 192, "rax", 0, 1, GCNFIELD_M_VDATA, ASMRVU_WRITE, 1 },
            { 192, "rcx4", 1, 5, GCNFIELD_M_VADDR, ASMRVU_READ, 1 },
            { 192, "srdx3", 0, 4, GCNFIELD_M_SRSRC, ASMRVU_READ, 4 },
            // image_get_lod rax, rcx4[1:4], srdx3[0:3], srcx5[4:7] dmask:1 unorm r128
            { 200, "rax", 0, 1, GCNFIELD_M_VDATA, ASMRVU_WRITE, 1 },
            { 200, "rcx4", 1, 5, GCNFIELD_M_VADDR, ASMRVU_READ, 1 },
            { 200, "srdx3", 0, 4, GCNFIELD_M_SRSRC, ASMRVU_READ, 4 },
            { 200, "srcx5", 4, 8, GCNFIELD_MIMG_SSAMP, ASMRVU_READ, 4 },
            /* image_atomic_fcmpswap rax2[5:6], rcx4[1:4], srdx3[0:3]
             * dmask:5 unorm glc r128 */
            { 208, "rax2", 5, 6, GCNFIELD_M_VDATA, ASMRVU_READ|ASMRVU_WRITE, 1 },
            { 208, "rcx4", 1, 5, GCNFIELD_M_VADDR, ASMRVU_READ, 1 },
            { 208, "srdx3", 0, 4, GCNFIELD_M_SRSRC, ASMRVU_READ, 4 },
            { 208, "rax2", 6, 7, GCNFIELD_M_VDATAH, ASMRVU_READ, 1 },
        },
        true, ""
    },
    {   /* 17: EXP encoding */
        ".regvar rax:v, rbx:v, rcx:v, rex:v\n"
        ".regvar rax2:v:10, rbx4:v:8, rcx4:v:12, rex5:v:10\n"
        "exp  param5, rax, rbx, rcx, rbx4[5] done vm\n"
        "exp  param5, off, rcx4[2], off, rbx4[6] done vm\n"
        "exp  param5, v54, v28, v83, v161 done vm\n"
        "exp  param5, off, v42, off, v97 done vm\n",
        {
            // exp  param5, rax, rbx, rcx, rbx4[5] done vm
            { 0, "rax", 0, 1, GCNFIELD_EXP_VSRC0, ASMRVU_READ, 1 },
            { 0, "rbx", 0, 1, GCNFIELD_EXP_VSRC1, ASMRVU_READ, 1 },
            { 0, "rcx", 0, 1, GCNFIELD_EXP_VSRC2, ASMRVU_READ, 1 },
            { 0, "rbx4", 5, 6, GCNFIELD_EXP_VSRC3, ASMRVU_READ, 1 },
            // exp  param5, off, rcx4[2], off, rbx4[65] done vm
            { 8, "rcx4", 2, 3, GCNFIELD_EXP_VSRC1, ASMRVU_READ, 1 },
            { 8, "rbx4", 6, 7, GCNFIELD_EXP_VSRC3, ASMRVU_READ, 1 },
            // exp  param5, v54, v28, v83, v161 done vm
            { 16, nullptr, 256+54, 256+55, GCNFIELD_EXP_VSRC0, ASMRVU_READ, 0 },
            { 16, nullptr, 256+28, 256+29, GCNFIELD_EXP_VSRC1, ASMRVU_READ, 0 },
            { 16, nullptr, 256+83, 256+84, GCNFIELD_EXP_VSRC2, ASMRVU_READ, 0 },
            { 16, nullptr, 256+161, 256+162, GCNFIELD_EXP_VSRC3, ASMRVU_READ, 0 },
            // exp  param5, off, v42, off, v97 done vm
            { 24, nullptr, 256+42, 256+43, GCNFIELD_EXP_VSRC1, ASMRVU_READ, 0 },
            { 24, nullptr, 256+97, 256+98, GCNFIELD_EXP_VSRC3, ASMRVU_READ, 0 }
        },
        true, ""
    }
};

static void testGCNRegVarUsages(cxuint i, const GCNRegVarUsageCase& testCase)
{
    std::istringstream input(testCase.input);
    std::ostringstream errorStream;
    
    Assembler assembler("test.s", input, ASM_ALL&~ASM_ALTMACRO,
                    BinaryFormat::GALLIUM, GPUDeviceType::CAPE_VERDE, errorStream);
    bool good = assembler.assemble();
    std::ostringstream oss;
    oss << " regVarUsageGCNCase#" << i;
    const std::string testCaseName = oss.str();
    assertValue<bool>("testGCNRegVarUsages", testCaseName+".good",
                      testCase.good, good);
    if (assembler.getSections().size()<1)
    {
        std::ostringstream oss;
        oss << "FAILED for " << " regVarUsageGCNCase#" << i;
        throw Exception(oss.str());
    }
    /*assertValue("testGCNRegVarUsages", testCaseName+".size",
                    testCase.regVarUsages.size(), section.regVarUsages.size());*/
    ISAUsageHandler* usageHandler = assembler.getSections()[0].usageHandler.get();
    usageHandler->rewind();
    size_t j;
    for (j = 0; usageHandler->hasNext(); j++)
    {
        assertTrue("testGCNRegVarUsages", testCaseName+"length",
                   j < testCase.regVarUsages.size());
        const AsmRegVarUsage resultRvu = usageHandler->nextUsage();
        std::ostringstream rvuOss;
        rvuOss << ".regVarUsage#" << j << ".";
        rvuOss.flush();
        std::string rvuName(rvuOss.str());
        const AsmRegVarUsageData& expectedRvu = testCase.regVarUsages[j];
        assertValue("testGCNRegVarUsages", testCaseName+rvuName+"offset",
                    expectedRvu.offset, resultRvu.offset);
        if (expectedRvu.regVarName==nullptr)
            assertTrue("testGCNRegVarUsages", testCaseName+rvuName+"regVarName",
                       resultRvu.regVar==nullptr);
        else // otherwise
            assertString("testGCNRegVarUsages", testCaseName+rvuName+"regVarName",
                        expectedRvu.regVarName, resultRvu.regVar->first);
        assertValue("testGCNRegVarUsages", testCaseName+rvuName+"rstart",
                    expectedRvu.rstart, resultRvu.rstart);
        assertValue("testGCNRegVarUsages", testCaseName+rvuName+"rend",
                    expectedRvu.rend, resultRvu.rend);
        assertValue("testGCNRegVarUsages", testCaseName+rvuName+"regField",
                    cxuint(expectedRvu.regField), cxuint(resultRvu.regField));
        assertValue("testGCNRegVarUsages", testCaseName+rvuName+"rwFlags",
                    cxuint(expectedRvu.rwFlags), cxuint(resultRvu.rwFlags));
        assertValue("testGCNRegVarUsages", testCaseName+rvuName+"align",
                    cxuint(expectedRvu.align), cxuint(resultRvu.align));
    }
    assertTrue("testGCNRegVarUsages", testCaseName+"length",
                   j == testCase.regVarUsages.size());
    assertString("testGCNRegVarUsages", testCaseName+".errorMessages",
              testCase.errorMessages, errorStream.str());
}

int main(int argc, const char** argv)
{
    int retVal = 0;
    for (size_t i = 0; i < sizeof(gcnRvuTestCases1Tbl)/sizeof(GCNRegVarUsageCase); i++)
        try
        { testGCNRegVarUsages(i, gcnRvuTestCases1Tbl[i]); }
        catch(const std::exception& ex)
        {
            std::cerr << ex.what() << std::endl;
            retVal = 1;
        }
    return retVal;
}
