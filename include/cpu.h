#pragma once

#include <cstdint>
#include <optional>

namespace Nes {
	class Memory;

	enum class Status {
		CARRY             = 0x01,
		ZERO              = 0x02,
		INTERRUPT_DISABLE = 0x04,
		DECIMAL           = 0x08,
		BREAK             = 0x10,
		OVERFLOW          = 0x40,
		NEGATIVE          = 0x80
	};

	enum class InterruptType {
		IRQ   = 0x01,
		NMI   = 0x02,
		RESET = 0x04
	};

	class Interrupt {
		public:
			Interrupt() = default;

			bool needs_handle() const;
			void set(InterruptType it, bool value);
			std::optional<uint16_t> get_handler();
		private:
			uint8_t _value;
	};

	struct Registers {
		uint16_t pc; /* Program Counter */
		uint8_t  sp; /* Stack Pointer */
		uint8_t  a; /* Accumulator */
		uint8_t  x; /* X Index Register */
		uint8_t  y; /* Y Index Register */
		uint8_t  s; /* Status register */

		void set_status(Status s, bool active);
		bool check_status(Status s) const;
		void set_zn_flags(uint8_t value);
	};

	class Cpu {
		public:
			Cpu(Memory &memory);
			void emulate();
		private:
			void push_stack(uint8_t byte);
			uint8_t pop_stack();

			void request_interrupt(InterruptType i);
			void handle_interrupt();

			uint16_t immediate();
			uint16_t absolute();
			uint16_t absolute_indexed(uint8_t index);
			uint16_t zeropage();
			uint16_t zeropage_indexed(uint8_t index);
			uint16_t indirect();
			uint16_t indexed_indirect();
			uint16_t indirect_indexed();
			uint8_t relative();

			void load(uint16_t addr, uint8_t& reg, uint8_t extra_cycles);
			void branch(uint8_t displacement, Status status, bool value_needed);
			void store(uint16_t addr, uint8_t reg, uint8_t extra_cycles);
			void adc(uint16_t addr, uint8_t extra_cycles, uint8_t ones_complement);
			void ana(uint16_t addr, uint8_t extra_cycles);
			void ora(uint16_t addr, uint8_t extra_cycles);
			void eor(uint16_t addr, uint8_t extra_cycles);
			void inc_dec(uint16_t addr, uint8_t change, uint8_t extra_cycles);
			void bit(uint16_t addr, uint8_t extra_cycles);
			void cmp(uint16_t addr, uint8_t reg, uint8_t extra_cycles);
			void lsr(uint16_t addr, uint8_t extra_cycles);
			void lsr_a();
			void asl(uint16_t addr, uint8_t extra_cycles);
			void asl_a();
			void ror(uint16_t addr, uint8_t extra_cycles);
			void ror_a();
			void rol(uint16_t addr, uint8_t extra_cycles);
			void rol_a();

			Memory& _mem;
			Registers _regs;
			Interrupt _interrupt;
			uint8_t _cycles;
			uint8_t _current_cycle;
	};
}
