/*
 * Author: Dakror
 */

#include <avr/io.h>
#include <math.h>
#include <stdlib.h>

#define confidence_threshhold 10

#define true 1
#define false 0

typedef uint8_t bool;

struct display {
	volatile uint8_t * sfr;
	uint8_t pin;
	bool mux;
} DSPA, DSPB, DSPC, DSPD;

struct button {
	volatile uint8_t * sfr;
	uint8_t pin;

	bool pressed;
	uint8_t pressed_confidence;
	uint8_t released_confidence;
};

char nums[] = {
// - - - - - - HDCGBEFA
        ' ', 0b00000000, //
        'E', 0b01010111, //
        '0', 0b01101111, //
        '1', 0b00101000, //
        '2', 0b01011101, //
        '3', 0b01111001, //
        '4', 0b00111010, //
        '5', 0b01110011, //
        '6', 0b01110111, //
        '7', 0b00101001, //
        '8', 0b01111111, //
        '9', 0b01111011, //
        };

uint8_t scene = 1;
uint8_t take = 1;

uint8_t * take_mem;

uint8_t tick = 0;

void debounce(struct button * b, void (*f)(void)) {
	if (bit_is_clear(*(b->sfr), b->pin)) {
		b->pressed_confidence++;

		if (b->pressed_confidence > confidence_threshhold) {
			if (!b->pressed) {
				(*f)();
				b->pressed = true;
			}

			b->pressed_confidence = 0;
		}
	} else {
		b->released_confidence++;

		if (b->released_confidence > confidence_threshhold) {
			b->pressed = false;
			b->released_confidence = 0;
		}
	}
}

uint8_t lookup(char num) {
	for (uint8_t i = 0; i < sizeof(nums); i += 2)
		if (num == nums[i]) return i + 1;

	return lookup('E'); // 'E' for error by default
}

char int2char(uint8_t num) {
	return (char) (((uint8_t) '0') + num);
}

void displayChar(char num, struct display * display) {
	*(display->sfr) = nums[lookup(num)];
	if (display->mux) {
		if (num == ' ') PORTB &= ~(1 << display->pin);
		else PORTB |= 1 << display->pin;
	}
}

void displayNumber(uint8_t number, bool take) {
	if (take) {
		displayChar(int2char(number % 10), &DSPD);
		displayChar(int2char(number / 10), &DSPC);
	} else {
		if (tick % 2 == 0) {
			displayChar(' ', &DSPA);
			displayChar(int2char(number % 10), &DSPB);
		} else {
			displayChar(' ', &DSPB);
			displayChar(int2char(number / 10), &DSPA);
		}
	}
}

void button0_press(void) {
	take = 1;
	take_mem[scene] = take;
	scene++;
	if (scene > 99) scene = 1;
}

void button1_press(void) {
	take++;
	if (take > 99) take = 1;
}

int main(void) {
	DSPA.sfr = &PORTD;
	DSPA.pin = PINB1;
	DSPA.mux = true;
	DSPB.sfr = &PORTD;
	DSPB.pin = PINB0;
	DSPB.mux = true;
	DSPC.sfr = &PORTA;
	DSPD.sfr = &PORTC;

	uint8_t s = sizeof(uint8_t);

	take_mem = malloc(100 * s);
	for (uint8_t i = 0; i < 100; i++)
		take_mem[i * s] = 0;

	// output mode
	DDRA = 0xff;

	DDRB = 0b00000011; // controller bits for mux displays
	DDRB &= ~(1 << PINB2);
	DDRB &= ~(1 << PINB3);
	DDRB &= ~(1 << PINB4);

	PORTB |= 1 << PINB2;
	PORTB |= 1 << PINB3;
	PORTB |= 1 << PINB4;

	DDRC = 0xff;
	DDRD = 0xff;


	struct button scene_button = { .sfr = &PINB, .pin = 2 };
	struct button take_button = { .sfr = &PINB, .pin = 3 };
	struct button lever_button = { .sfr = &PINB, .pin = 4 };

	while (true) {
		debounce(&scene_button, button0_press);

		debounce(&take_button, button1_press);
		debounce(&lever_button, button1_press);

		displayNumber(take, true);
		displayNumber(scene, false);

		tick = ++tick % 256;
	}

	free(take_mem);

	return 0;
}

