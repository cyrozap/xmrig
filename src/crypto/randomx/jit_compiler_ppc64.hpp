/*
Copyright (c) 2023 tevador <tevador@gmail.com>
Copyright (c) 2026, Forest Crossman <cyrozap@gmail.com>

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
	* Redistributions of source code must retain the above copyright
	  notice, this list of conditions and the following disclaimer.
	* Redistributions in binary form must reproduce the above copyright
	  notice, this list of conditions and the following disclaimer in the
	  documentation and/or other materials provided with the distribution.
	* Neither the name of the copyright holder nor the
	  names of its contributors may be used to endorse or promote products
	  derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

#include <vector>

#include "crypto/randomx/common.hpp"
#include "crypto/randomx/jit_compiler_ppc64_static.hpp"

#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	#define PPC_BIG_ENDIAN 1
#else
	#define PPC_BIG_ENDIAN 0
#endif

#if (defined(_CALL_ELF) && _CALL_ELF == 2) || (!defined(_CALL_ELF) && !PPC_BIG_ENDIAN)
	#define PPC_ABI_V2 1
#else
	#define PPC_ABI_V2 0
#endif

namespace randomx {

	struct CodeBuffer {
		uint8_t* code;
		int32_t codePos;
		int32_t rcpCount;

		void emit(const uint8_t* src, int32_t len) {
			memcpy(&code[codePos], src, len);
			codePos += len;
		}

		template<typename T>
		void emit(T src) {
			memcpy(&code[codePos], &src, sizeof(src));
			codePos += sizeof(src);
		}

		void emitAt(int32_t codePos, const uint8_t* src, int32_t len) {
			memcpy(&code[codePos], src, len);
		}

		template<typename T>
		void emitAt(int32_t codePos, T src) {
			memcpy(&code[codePos], &src, sizeof(src));
		}
	};

	struct CompilerState : public CodeBuffer {
		int32_t instructionOffsets[RANDOMX_PROGRAM_MAX_SIZE];
		int registerUsage[RegistersCount];
	};

	class Program;
	struct ProgramConfiguration;
	class SuperscalarProgram;
	class Instruction;

#define HANDLER_ARGS randomx::JitCompilerPPC64* jit, randomx::CompilerState& state, randomx::Instruction isn, int i
	typedef void(*InstructionGeneratorPPC64)(HANDLER_ARGS);

	class JitCompilerPPC64 {
	public:
		JitCompilerPPC64(bool hugePagesEnable, bool optimizedInitDatasetEnable);
		~JitCompilerPPC64();

		void prepare() {}
		void generateProgram(Program&, ProgramConfiguration&, uint32_t);
		void generateProgramLight(Program&, ProgramConfiguration&, uint32_t);

		template<size_t N>
		void generateSuperscalarHash(SuperscalarProgram(&programs)[N]);

		void generateDatasetInitCode() {}

		ProgramFunc* getProgramFunc() {
#if PPC_ABI_V2
			return reinterpret_cast<ProgramFunc*>(entryProgram);
#else
			return reinterpret_cast<ProgramFunc*>(descriptorProgram);
#endif
		}
		DatasetInitFunc* getDatasetInitFunc() {
#if PPC_ABI_V2
			return reinterpret_cast<DatasetInitFunc*>(entryDataInit);
#else
			return reinterpret_cast<DatasetInitFunc*>(descriptorDataInit);
#endif
		}
		uint8_t* getCode() { return state.code; }
		size_t getCodeSize();

		void enableWriting() const;
		void enableExecution() const;

		static InstructionGeneratorPPC64 engine[256];
		static uint8_t inst_map[256];

	private:
		void emitProgramPrefix(CompilerState& state, Program& prog, ProgramConfiguration& pcfg);
		void emitProgramSuffix(CompilerState& state, ProgramConfiguration& pcfg);

		uint32_t getTempGpr();
		uint32_t getTempVr();

		CompilerState state;
		randomx_flags flags;

		void* entryDataInit = nullptr;
		void* entryProgram = nullptr;
#if !PPC_ABI_V2
		uint64_t descriptorProgram[3];
		uint64_t descriptorDataInit[3];
#endif

		int32_t RandomXCodePos;
		int32_t SshashSingleItemPos;
		int32_t LoopBeginPos;

		uint32_t tempGprIndex = 0;
		uint32_t tempVrIndex = 0;

	public:
		static void v1_IADD_RS(HANDLER_ARGS);
		static void v1_IADD_M(HANDLER_ARGS);
		static void v1_ISUB_R(HANDLER_ARGS);
		static void v1_ISUB_M(HANDLER_ARGS);
		static void v1_IMUL_R(HANDLER_ARGS);
		static void v1_IMUL_M(HANDLER_ARGS);
		static void v1_IMULH_R(HANDLER_ARGS);
		static void v1_IMULH_M(HANDLER_ARGS);
		static void v1_ISMULH_R(HANDLER_ARGS);
		static void v1_ISMULH_M(HANDLER_ARGS);
		static void v1_IMUL_RCP(HANDLER_ARGS);
		static void v1_INEG_R(HANDLER_ARGS);
		static void v1_IXOR_R(HANDLER_ARGS);
		static void v1_IXOR_M(HANDLER_ARGS);
		static void v1_IROR_R(HANDLER_ARGS);
		static void v1_IROL_R(HANDLER_ARGS);
		static void v1_ISWAP_R(HANDLER_ARGS);
		static void v1_FSWAP_R(HANDLER_ARGS);
		static void v1_FADD_R(HANDLER_ARGS);
		static void v1_FADD_M(HANDLER_ARGS);
		static void v1_FSUB_R(HANDLER_ARGS);
		static void v1_FSUB_M(HANDLER_ARGS);
		static void v1_FSCAL_R(HANDLER_ARGS);
		static void v1_FMUL_R(HANDLER_ARGS);
		static void v1_FDIV_M(HANDLER_ARGS);
		static void v1_FSQRT_R(HANDLER_ARGS);
		static void v1_CBRANCH(HANDLER_ARGS);
		static void v1_CFROUND(HANDLER_ARGS);
		static void v1_ISTORE(HANDLER_ARGS);
		static void v1_NOP(HANDLER_ARGS);
	};

}
