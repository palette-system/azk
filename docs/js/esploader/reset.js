/**
 * Sleep for ms milliseconds
 * @param {number} ms Milliseconds to wait
 * @returns {Promise<void>}
 */
function sleep(ms) {
    return new Promise((resolve) => setTimeout(resolve, ms));
}
/**
 * Execute a classic set of commands that will reset the chip.
 *
 * Commands (e.g. R0) are defined by a code (R) and an argument (0).
 *
 * The commands are:
 *
 * D: setDTR - 1=True / 0=False
 *
 * R: setRTS - 1=True / 0=False
 *
 * W: Wait (time delay) - positive integer number (miliseconds)
 *
 * "D0|R1|W100|D1|R0|W50|D0" represents the classic reset strategy
 * @param {Transport} transport Transport class to perform serial communication.
 * @param {number} resetDelay Delay in milliseconds for reset.
 */
export class ClassicReset {
    constructor(transport, resetDelay) {
        this.resetDelay = resetDelay;
        this.transport = transport;
    }
    async reset() {
        await this.transport.setDTR(false);
        await this.transport.setRTS(true);
        await sleep(100);
        await this.transport.setDTR(true);
        await this.transport.setRTS(false);
        await sleep(this.resetDelay);
        await this.transport.setDTR(false);
    }
}
/**
 * Execute a set of commands for USB JTAG serial reset.
 *
 * Commands (e.g. R0) are defined by a code (R) and an argument (0).
 *
 * The commands are:
 *
 * D: setDTR - 1=True / 0=False
 *
 * R: setRTS - 1=True / 0=False
 *
 * W: Wait (time delay) - positive integer number (miliseconds)
 * @param {Transport} transport Transport class to perform serial communication.
 */
export class UsbJtagSerialReset {
    constructor(transport) {
        this.transport = transport;
    }
    async reset() {
        await this.transport.setRTS(false);
        await this.transport.setDTR(false);
        await sleep(100);
        await this.transport.setDTR(true);
        await this.transport.setRTS(false);
        await sleep(100);
        await this.transport.setRTS(true);
        await this.transport.setDTR(false);
        await this.transport.setRTS(true);
        await sleep(100);
        await this.transport.setRTS(false);
        await this.transport.setDTR(false);
    }
}
/**
 * Execute a set of commands that will hard reset the chip.
 *
 * Commands (e.g. R0) are defined by a code (R) and an argument (0).
 *
 * The commands are:
 *
 * D: setDTR - 1=True / 0=False
 *
 * R: setRTS - 1=True / 0=False
 *
 * W: Wait (time delay) - positive integer number (miliseconds)
 * @param {Transport} transport Transport class to perform serial communication.
 * @param {boolean} usingUsbOtg is it using USB-OTG ?
 */
export class HardReset {
    constructor(transport, usingUsbOtg = false) {
        this.transport = transport;
        this.usingUsbOtg = usingUsbOtg;
        this.transport = transport;
    }
    async reset() {
        if (this.usingUsbOtg) {
            await sleep(200);
            await this.transport.setRTS(false);
            await sleep(200);
        }
        else {
            await sleep(100);
            await this.transport.setRTS(false);
        }
    }
}
/**
 * Validate a sequence string based on the following format:
 *
 * Commands (e.g. R0) are defined by a code (R) and an argument (0).
 *
 * The commands are:
 *
 * D: setDTR - 1=True / 0=False
 *
 * R: setRTS - 1=True / 0=False
 *
 * W: Wait (time delay) - positive integer number (miliseconds)
 * @param {string} seqStr Sequence string to validate
 * @returns {boolean} Is the sequence string valid ?
 */
export function validateCustomResetStringSequence(seqStr) {
    const commands = ["D", "R", "W"];
    const commandsList = seqStr.split("|");
    for (const cmd of commandsList) {
        const code = cmd[0];
        const arg = cmd.slice(1);
        if (!commands.includes(code)) {
            return false; // Invalid command code
        }
        if (code === "D" || code === "R") {
            if (arg !== "0" && arg !== "1") {
                return false; // Invalid argument for D and R commands
            }
        }
        else if (code === "W") {
            const delay = parseInt(arg);
            if (isNaN(delay) || delay <= 0) {
                return false; // Invalid argument for W command
            }
        }
    }
    return true; // All commands are valid
}
/**
 * Custom reset strategy defined with a string.
 *
 * The sequenceString input string consists of individual commands divided by "|".
 *
 * Commands (e.g. R0) are defined by a code (R) and an argument (0).
 *
 * The commands are:
 *
 * D: setDTR - 1=True / 0=False
 *
 * R: setRTS - 1=True / 0=False
 *
 * W: Wait (time delay) - positive integer number (miliseconds)
 *
 * "D0|R1|W100|D1|R0|W50|D0" represents the classic reset strategy
 * @param {Transport} transport Transport class to perform serial communication.
 * @param {string} sequenceString Custom string sequence for reset strategy
 */
export class CustomReset {
    constructor(transport, sequenceString) {
        this.transport = transport;
        this.sequenceString = sequenceString;
        this.transport = transport;
    }
    async reset() {
        const resetDictionary = {
            D: async (arg) => await this.transport.setDTR(arg),
            R: async (arg) => await this.transport.setRTS(arg),
            W: async (delay) => await sleep(delay),
        };
        try {
            const isValidSequence = validateCustomResetStringSequence(this.sequenceString);
            if (!isValidSequence) {
                return;
            }
            const cmds = this.sequenceString.split("|");
            for (const cmd of cmds) {
                const cmdKey = cmd[0];
                const cmdVal = cmd.slice(1);
                if (cmdKey === "W") {
                    await resetDictionary["W"](Number(cmdVal));
                }
                else if (cmdKey === "D" || cmdKey === "R") {
                    await resetDictionary[cmdKey](cmdVal === "1");
                }
            }
        }
        catch (error) {
            throw new Error("Invalid custom reset sequence");
        }
    }
}
export default { ClassicReset, CustomReset, HardReset, UsbJtagSerialReset, validateCustomResetStringSequence };
