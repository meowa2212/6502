PORTB = $6000
PORTA = $6001
DDRB  = $6002
DDRA  = $6003

E  = %10000000
RW = %01000000
RS = %00100000

    .org $8000
reset:
    ldx #$ff
    txs

    lda #%11111111 ; set all portb pins to output
    sta DDRB
    lda #%11100000 ; set top 3 pins to output
    sta DDRA

    lda #%00111000 ; 8-bit mode, 2-line display, 5x8 font
    jsr lcd_instruction
    lda #%00001110 ; display on, cursor on, blink off
    jsr lcd_instruction
    lda #%00000110 ; increment and shift cursor, dont shift display
    jsr lcd_instruction
    lda #%00000001 ; clear display
    jsr lcd_instruction

    ldx #0
print:
    lda message,x
    beq loop
    jsr print_char
    inx
    jmp print
    
loop:
    jmp loop ; infinite loop to keep the program running

message: .asciiz "  Siema mordo,                             Co u ciebie?"

lcd_wait:
    pha
    lda #%00000000 ; Port B is input
    sta DDRB
lcd_busy:
    lda #RW
    sta PORTA
    lda #(RW | E)
    sta PORTA
    lda PORTB
    and #%10000000 ; getting only the bit we care about
    bne lcd_busy

    lda #RW
    sta PORTA
    lda #%11111111 ; Port B is output
    sta DDRB
    pla
    rts

lcd_instruction:
    jsr lcd_wait   ; waiting until busy flag is off
    sta PORTB
    lda #0         ; clear RS/RW/E bits
    sta PORTA
    lda #E         ; set E bit to send instruction
    sta PORTA
    lda #0         ; clear RS/RW/E bits
    sta PORTA
    rts

print_char:
    jsr lcd_busy   ; waiting until busy flag is off
    sta PORTB
    lda #RS
    sta PORTA
    lda #(RS | E)
    sta PORTA
    lda #RS
    sta PORTA
    rts

    .org $fffc ; stores the start vector
    .word reset
    .word $0000