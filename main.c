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

uint8_t scene = 0;
uint8_t take = 0;

uint8_t * take_mem;

uint8_t tick = 0;

bool display_on;

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

char int_to_char(uint8_t num) {
	return (char) (((uint8_t) '0') + num);
}

void display_char(char num, struct display * display) {
	*(display->sfr) = nums[lookup(num)];

	PORTA |= 1 << PINA7; // button output
	PORTD |= 1 << PIND7; // button output

	if (display->mux) {
		if (num == ' ') PORTB &= ~(1 << display->pin);
		else PORTB |= 1 << display->pin;
	}
}

void display_number(uint8_t number, bool take) {
	if (take) {
		display_char(int_to_char(number % 10), &DSPD);
		display_char(int_to_char(number / 10), &DSPC);
	} else {
		if (tick % 2 == 0) {
			display_char(' ', &DSPA);
			display_char(int_to_char(number % 10), &DSPB);
		} else {
			display_char(' ', &DSPB);
			display_char(int_to_char(number / 10), &DSPA);
		}
	}
}

void scene_incr(void) {
	take_mem[scene * sizeof(uint8_t)] = take;
	scene++;
	if (scene > 99) scene = 0;
	take = take_mem[scene * sizeof(uint8_t)];
}

void scene_decr(void) {
	take_mem[scene * sizeof(uint8_t)] = take;
	scene--;
	if (scene > 99 ) scene = 99;
	take = take_mem[scene * sizeof(uint8_t)];
}

void take_incr(void) {
	take++;
	if (take > 99) take = 0;
}

void take_decr(void) {
	take--;
	if (take > 99) take = 99;
}

void toggle_display(void) {
	display_on = !display_on;
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

	DDRA = 0b01111111;
	DDRA &= ~(1 << PINA7);

	display_on = true;

	DDRB = 0b00000011; // controller bits for mux displays
	DDRB &= ~(1 << PINB2);
	DDRB &= ~(1 << PINB3);
	DDRB &= ~(1 << PINB4);

	PORTB |= 1 << PINB2;
	PORTB |= 1 << PINB3;
	PORTB |= 1 << PINB4;

	DDRC = 0xff;

	DDRD = 0b01111111;
	DDRD &= ~(1 << PIND7);

	struct button button_scene_incr = { .sfr = &PINA, .pin = 7 };
	struct button button_scene_decr = { .sfr = &PINB, .pin = 4 };

	struct button button_take_incr = { .sfr = &PINB, .pin = 3 };
	struct button button_take_decr = { .sfr = &PINB, .pin = 2 };

	struct button button_display_toggle = { .sfr = &PIND, .pin = 7 };

	while (true) {
		debounce(&button_display_toggle, toggle_display);

		if (display_on) {
			debounce(&button_scene_incr, scene_incr);
			debounce(&button_scene_decr, scene_decr);

			debounce(&button_take_incr, take_incr);
			debounce(&button_take_decr, take_decr);

			display_number(take, true);
			display_number(scene, false);
		} else {
			display_char(' ', &DSPA);
			display_char(' ', &DSPB);
			display_char(' ', &DSPC);
			display_char(' ', &DSPD);
		}

		tick++;
		tick %= 256;
	}

	free(take_mem);

	return 0;
}

