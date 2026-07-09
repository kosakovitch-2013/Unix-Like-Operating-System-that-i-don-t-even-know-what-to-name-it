void _start() {
    char* vga = (char*)0xB8000;
    
    // Simple shell loop - write prompt, wait for key (polling)
    while(1) {
        vga[0] = '#';
        vga[1] = 0x0F;
        vga[2] = ' ';
        vga[3] = 0x0F;
        
        // Read keyboard
        char c = *(volatile char*)0x60;  // Keyboard data port
        
        if (c) {
            vga[4] = c;
            vga[5] = 0x0F;
        }
        
        for(volatile int i = 0; i < 1000000; i++);  // Delay
    }
}
