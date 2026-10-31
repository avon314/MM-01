# Plan: Modbus write behaviour for FC05 / FC06 / FC15 (read-before-write)

## Context
Today a server `"WR":[DPN,"HEX"]` command is parsed to 2 bytes (`Extract_WR_Data_From_JSON()`,
json_extraction.c — unchanged) and written as-is by `Write_to_Register_Functionality()` (modbus_app.c)
with the data point's configured function. Required behaviour:
- **FC05 (write single coil):** read the coil from the slave (FC01, same slave and address as the write
  data point), invert it, write. Server value ignored — any WR toggles.
- **FC06 (write single register):** read the register (FC03, same slave and address), replace only the
  bytes covered by the data length, keep the rest, write. Server always sends 2 bytes.
  - data length 0/1 (1 byte): keep the read high byte, write received low byte.
  - data length 2 and 3: write the 2 received bytes to the configured register (same as current code;
    4-byte data points are not split over two registers).
- **FC15 (write multiple coils):** overwrite — unchanged from current code.

## Design
New op state `MB_OP_WRITE_READ` between the request and the write:
1. `Write_to_Register_Functionality()` keeps its validation. Stores slave, address, FC, data length and
   the 2 received bytes in a static write context.
   - FC05 → `modbus_frame(slave, 0x01, reg, 1, 0, …)`; FC06 → `modbus_frame(slave, 0x03, reg, 1, 0, …)`;
     state `MB_OP_WRITE_READ`.
   - FC15 → as today, state `MB_OP_WRITE`.
2. Read response → new `Process_Write_Read_Response()`: check slave, read FC and byte count; build the
   FC05 value (`00 00` if ON, `FF 00` if OFF) or the merged FC06 value; `modbus_frame()` + `send_frame()`;
   state `MB_OP_WRITE`. Mismatching response → resume polling.
3. Write ack → `Record_Modbus_Write_Status(slave, 0)` + resume polling (as today).
4. Failures: exception to the read → `Record_Modbus_Write_Status(slave, code)` ("ME" write error) and
   resume; no reply after retries → `send_frame()` failure path resumes polling for `MB_OP_WRITE_READ`
   like `MB_OP_WRITE`; CRC error → existing resume path.
New helper `Resume_DPT_Polling()` (`mb_poll_dpt = 1`, `gb_mb_write_processing_f = 0`,
`modbus_op_state = MB_OP_DPT_READ`) replaces the repeated three lines.

## Changes
1. `src/Application Files/Modbus Application/modbus_app.h`: add `MB_OP_WRITE_READ` to `MB_OP_STATE`;
   prototypes `Process_Write_Read_Response()`, `Resume_DPT_Polling()`.
2. `src/Application Files/Modbus Application/modbus_app.c` (Windows-1252 → byte-safe Perl edits):
   write context struct; FC05/FC06 branch in `Write_to_Register_Functionality()`;
   `Process_Write_Read_Response()`; `Resume_DPT_Polling()`; task switches: valid-response
   `case MB_OP_WRITE_READ` → `Process_Write_Read_Response()`, `MB_OP_WRITE` ack → record + resume;
   exception `case MB_OP_WRITE` / `MB_OP_WRITE_READ` → record write error + resume.
3. `src/MODBUS Driver Files/Modbus.c`: `send_frame()` retries-exhausted path handles
   `MB_OP_WRITE_READ` like `MB_OP_WRITE` via `Resume_DPT_Polling()`; CRC-error path in `frame_rcvd()`
   uses `Resume_DPT_Polling()`.

Reuse: `modbus_frame()` / `send_frame()` (Modbus.c already builds FC01/03/05/06/0F),
`Segregate_DPT_Data_To_Buffer()`, `Record_Modbus_Write_Status()`.

## Verification
1. `arm-none-eabi-gcc -fsyntax-only` with Debug/Makefile flags on modbus_app.c and Modbus.c — no errors,
   no new warnings in changed code.
2. On target with a slave simulator (debug print `Response received from slave`):
   - FC05, coil OFF, any WR → FC01 read, then FC05 `FF 00`; again → FC05 `00 00`.
   - FC06 data length 1, register `12 AB`, WR `"00CD"` → FC03 read, then FC06 `12 CD`.
   - FC06 data length 2 or 3, WR `"BEEF"` → FC03 read, then FC06 `BE EF` at the configured address.
   - FC15 → unchanged frame vs. current firmware.
   - Read gets exception (e.g. `03 83 02`) → "ME" `[3,0,2]`, polling resumes.
   - Slave silent on the read → after retries polling resumes, slave stays active.
