/*
 * Author: Dakror
 */

#include <avr/io.h>
#include <util/delay.h>
#include <math.h>

#define confidence_threshhold 10

// -- displays -- //
#define DSPA PINA0
#define DSPB PINA1
#define DSPC PINA2
#define DSPD PINA3

const uint8_t take_displays[] = { DSPC, DSPD };
const uint8_t scene_displays[] = { DSPA };

struct btn {
	volatile uint8_t * sfr;
	uint8_t pin;

	/**
	 0 = not pressed
	 1 = was pressed
	 **/
	uint8_t pressed;
	uint8_t pressed_confidence;
	uint8_t released_confidence;
};

// -- chars -- //

char nums[] = {
// - - - - - - HDCGBEFA
        ' ', 0b00000000, //
        '.', 0b10000000, //
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

int lookup(char num) {
	for (uint8_t i = 0; i < sizeof(nums); i += 2)
		if (num == nums[i]) return i + 1;

	return lookup('E'); // 'E' for error by default
}

// -- display -- //

void displayIndex(uint8_t index, uint8_t port) {
	PORTC = nums[index];
	PORTA = 0b11111111; // all other pins of a are off
}

void display(char num, uint8_t port) {
	displayIndex(lookup(num), port);
}

void err(void) {
	display('E', DSPA);
	display('E', DSPB);
	display('E', DSPC);
	display('E', DSPD);
}

void displayNumber(uint8_t num, const uint8_t ports[], uint8_t portsc) {
	uint8_t digits = floor(log10(num)) + 1;

	if (digits > portsc) {
		err();
		return;
	}

	if (num == 0) digits = 1;

	for (uint8_t i = 0; i < portsc; i++) {
		display((char) (((uint8_t) '0') + (num % 10)), ports[(portsc - 1) - i]);
		num /= 10;
	}
}

// -- button -- //

void debounce(struct btn * b, void (*f)(void)) {
	if (bit_is_clear(*(b->sfr), b->pin)) {
		b->pressed_confidence++;

		if (b->pressed_confidence > confidence_threshhold) {
			if (b->pressed == 0) {
				(*f)();
				b->pressed = 1;
			}

			b->pressed_confidence = 0;
		}
	} else {
		b->released_confidence++;
		if (b->released_confidence > confidence_threshhold) {
			b->pressed = 0;
			b->released_confidence = 0;
		}
	}
}

// -- main part -- //

uint8_t scene = 1;
uint8_t take = 1;

void btn0_press(void) {
	take++;
	if (take > 99) take = 1;
}

void btn1_press(void) {
	take = 1;
	scene++;
	if (scene > 9) scene = 1;
}

int main(void) {
	DDRB &= ~(1 << PINB0); // input (inverted)
	DDRB &= ~(1 << PINB1); // input (inverted)
	PORTB = 1 << PINB0 | 1 << PINB1;

	DDRA = 0b11111111; // full a part on output
	DDRC = 0b11111111; // full c part on output

	/*struct btn btn0 = { .sfr = &PINB, .pin = 0, .pressed = 0, .pressed_confidence = 0, .released_confidence = 0 };
	struct btn btn1 = { .sfr = &PINB, .pin = 1, .pressed = 0, .pressed_confidence = 0, .released_confidence = 0 };
	 */

	display('8', DSPC);

	while (1) {
		/*displayNumber(take, take_displays, 2);
		displayNumber(scene, scene_displays, 1);

		debounce(&btn0, btn0_press);
		debounce(&btn1, btn1_press);*/
	}

	return 0;
}
