xxd -r -p flash.hex flash.bin
xxd -r -p flashErase.hex flashErase.bin
xxd -r -p reset.hex reset.bin
xxd -r -p flashQuery.hex flashQuery.bin
cat flash.bin trail.bin > flashTrail.bin
cat flash.bin blink.bin > flashBlink.bin
cat flash.bin zeros.bin > flashZeros.bin

