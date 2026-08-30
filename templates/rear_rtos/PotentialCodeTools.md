### The GCC/Clang Attribute: \__attribute__((packed)) 

When using a GCC or Clang toolchain (standard in CMake/Ninja environments), this tells the compiler to pack only the specific struct it is attached to. This is the safer, more modern approach because it eliminates the risk of accidentally packing other structs in your codebase.
```
C
typedef struct __attribute__((packed)) {
    uint16_t engine_rpm;
    uint8_t  throttle_pedal;
    uint32_t cvt_belt_temp;
} BajaEcuSignals_t;
```

### Multiplexed Static Frames
If you need the determinism of static slots but the bandwidth efficiency of dynamic packing, the optimal architectural choice is Multiplexing.

How it Works: You define a static frame, but dedicate Byte 0 as a "Multiplexor ID". If Byte 0 is 0x01, the remaining bytes contain wheel speeds. If Byte 0 is 0x02, the remaining bytes contain suspension travel.

The Advantage: This gives you the flexibility to rotate lower-priority data through a single CAN ID, saving bandwidth while remaining entirely predictable and fully supported by DBC files and standard parsing tools.