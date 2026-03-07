#!/bin/bash
# generate_compile_commands.sh
echo "[" >compile_commands.json
for f in $(find src -name "*.c"); do
  echo "  {" >>compile_commands.json
  echo "    \"directory\": \"$(pwd)\"," >>compile_commands.json
  echo "    \"command\": \"gcc -mmcu=\$MCU -DF_CPU=\$F_CPU -Os -Isrc -Isrc/display -Isrc/display/bus -Isrc/display/controller -Isrc/display/internal -Isrc/I2C -Isrc/UART -c $f\"," >>compile_commands.json
  echo "    \"file\": \"$f\"" >>compile_commands.json
  echo "  }," >>compile_commands.json
done
# Remove last comma and close array
sed -i '$ s/,$//' compile_commands.json
echo "]" >>compile_commands.json
