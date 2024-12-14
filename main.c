/*
 * Step9.c
 *
 * Created: 6/21/2024 3:35:08 PM
 * Author : hasar
 */ 

#include <avr/io.h>
#include <util/delay.h>
#include <math.h>  // For round() function
#include <avr/interrupt.h> // For timer interrupts

// Define pins for SSD segments
#define SHC PD1
#define STC PD2
#define DS PD7

// Define pins for digit control
#define DIGIT1 PB2
#define DIGIT2 PB4
#define DIGIT3 PB5
#define DIGIT4 PB0

// Define push button pins
#define INCREMENT_BUTTON PC5
#define DECREMENT_BUTTON PC4
#define CONVERT_BUTTON PC3
#define ALARM_STOP PC2
#define MODE_BUTTON PC1

// Define LED pins
#define LED1 PB1
#define LED2 PD3
#define LED3 PD5
#define LED4 PD6
#define LED5 PB3

// Define temperature thresholds
#define CELSIUS_THRESHOLD 40

// Timer counter
volatile uint8_t timerCounter = 0;
volatile uint8_t ledBlinking = 0;
volatile uint8_t ledDisabledByButton = 0; // Flag to track LED disabled state
volatile uint8_t mode = 1; // Start in mode 1

// Variable to track Celsius temperature starting from 27
volatile uint8_t temperatureValue = 27;

// Function to send data to the shift register
void shiftOut(uint8_t data) {
	for (int i = 0; i < 8; i++) {
		if (data & (1 << (7 - i)))
		PORTD |= (1 << DS);
		else
		PORTD &= ~(1 << DS);

		PORTD |= (1 << SHC);
		_delay_us(1);
		PORTD &= ~(1 << SHC);
	}

	PORTD |= (1 << STC);
	_delay_us(1);
	PORTD &= ~(1 << STC);
}

// Function to display a number on the SSD
void displayNumber(uint8_t digit, uint8_t number) {
	shiftOut(number);
	if (digit == 1) {
		PORTB |= (1 << DIGIT1);
		_delay_ms(5);
		PORTB &= ~(1 << DIGIT1);
		} else if (digit == 2) {
		PORTB |= (1 << DIGIT2);
		_delay_ms(5);
		PORTB &= ~(1 << DIGIT2);
		} else if (digit == 3) {
		PORTB |= (1 << DIGIT3);
		_delay_ms(5);
		PORTB &= ~(1 << DIGIT3);
		} else if (digit == 4) {
		PORTB |= (1 << DIGIT4);
		_delay_ms(5);
		PORTB &= ~(1 << DIGIT4);
	}
}

void initTimer(void) {
	// Set Timer1 to CTC mode
	TCCR1B |= (1 << WGM12);
	// Set compare value for 200ms interval at 1MHz clock with 1024 prescaler
	OCR1A = 195;
	// Enable compare interrupt
	TIMSK1 |= (1 << OCIE1A);
	// Set prescaler to 1024 and start the timer
	TCCR1B |= (1 << CS12) | (1 << CS10);
}

void initPWMTimer2(void) {
	// Set PD3 as output
	DDRD |= (1 << LED2);
	// Set Fast PWM mode with non-inverted output
	TCCR2A |= (1 << WGM21) | (1 << WGM20) | (1 << COM2B1);
	// Set prescaler to 64
	TCCR2B |= (1 << CS22);
}

void initPWMTimer0(void) {
	// Set PD5 as output
	DDRD |= (1 << LED3);
	// Set Fast PWM mode with non-inverted output
	TCCR0A |= (1 << WGM01) | (1 << WGM00) | (1 << COM0B1);
	// Set prescaler to 64
	TCCR0B |= (1 << CS01) | (1 << CS00);
}

void initPWMTimer4(void) {
	// Set PD6 as output
	DDRD |= (1 << LED4);
	// Set Fast PWM mode with non-inverted output
	TCCR0A |= (1 << WGM01) | (1 << WGM00) | (1 << COM0A1);
	// Set prescaler to 64
	TCCR0B |= (1 << CS01) | (1 << CS00);
}

void initPWMTimer1(void) {
	// Set PB3 as output
	DDRB |= (1 << LED5);
	// Set Fast PWM mode with non-inverted output
	TCCR2A |= (1 << WGM20) | (1 << WGM21) | (1 << COM2A1);
	// Set prescaler to 64
	TCCR2B |= (1 << CS22);
}

// Timer1 compare match interrupt service routine
ISR(TIMER1_COMPA_vect) {
	timerCounter++;
	if (ledBlinking && timerCounter >= 5 && !ledDisabledByButton) { // 5 * 200ms = 1000ms
		if (temperatureValue <= CELSIUS_THRESHOLD) {
			// If temperature is below or at threshold, turn off LED
			PORTB &= ~(1 << LED1);
			} else {
			// Toggle LED if temperature is above threshold
			PORTB ^= (1 << LED1);
		}
		timerCounter = 0;
	}
}

void updateLEDBrightness(uint16_t value, uint8_t isCelsius) {
	uint8_t brightness = 0;

	if (isCelsius) {
		if (value <= 15) {
			brightness = (value * 255) / 15; // Scale value to 0-255 range
			} else {
			brightness = 255; // Maximum brightness
		}
	}
	OCR2B = brightness; // Set PWM duty cycle
}

void updateLEDBrightness3(uint16_t value, uint8_t isCelsius) {
	uint8_t brightness = 0;

	if (isCelsius) {
		if (value <= 16) {
			brightness = 0; // Minimum brightness
			} else if (value <= 25) {
			brightness = ((value - 16) * 255) / 9; // Scale value to 0-255 range
			} else {
			brightness = 255; // Maximum brightness
		}
	}
	OCR0B = brightness; // Set PWM duty cycle
}

void updateLEDBrightness4(uint16_t value, uint8_t isCelsius) {
	uint8_t brightness = 0;

	if (isCelsius) {
		if (value <= 26) {
			brightness = 0; // Minimum brightness
			} else if (value <= 35) {
			brightness = ((value - 26) * 255) / 9; // Scale value to 0-255 range
			} else {
			brightness = 255; // Maximum brightness
		}
	}

	OCR0A = brightness; // Set PWM duty cycle
}

void updateLEDBrightness5(uint16_t value, uint8_t isCelsius) {
	uint8_t brightness = 0;

	if (isCelsius) {
		if (value < 36) {
			brightness = 0; // Minimum brightness
			} else if (value <= 40) {
			brightness = ((value - 36) * 255) / 4; // Scale value to 0-255 range
			} else {
			brightness = 255; // Maximum brightness
		}
	}
	OCR2A = brightness; // Set PWM duty cycle
}

void incrementTemperatureValue(void) {
	if (!(PINC & (1 << INCREMENT_BUTTON))) {
		_delay_ms(20); // Debounce delay
		if (!(PINC & (1 << INCREMENT_BUTTON))) { // Confirm the button is still pressed
			// Increment the value
			temperatureValue++;
			if (temperatureValue > 99) {
				temperatureValue = 99; // Cap at two digits
			}

			// Wait for the button to be released
			while (!(PINC & (1 << INCREMENT_BUTTON))) {
				_delay_ms(20);
			}
		}
	}
}

void decrementTemperatureValue(void) {
	if (!(PINC & (1 << DECREMENT_BUTTON))) {
		_delay_ms(20); // Debounce delay
		if (!(PINC & (1 << DECREMENT_BUTTON))) { // Confirm the button is still pressed
			// Decrement the value
			if (temperatureValue > 0) {
				temperatureValue--;
			}

			// Wait for the button to be released
			while (!(PINC & (1 << DECREMENT_BUTTON))) {
				_delay_ms(20);
			}
		}
	}
}

int main(void) {
	// Set SSD control pins as output
	DDRD |= (1 << SHC) | (1 << STC) | (1 << DS);

	// Set digit control pins as output
	DDRB |= (1 << DIGIT1) | (1 << DIGIT2) | (1 << DIGIT3) | (1 << DIGIT4);

	// Set push button pins as input
	DDRC &= ~((1 << INCREMENT_BUTTON) | (1 << DECREMENT_BUTTON) | (1 << CONVERT_BUTTON) | (1 << ALARM_STOP) | (1 << MODE_BUTTON));
	// Enable pull-up resistors for push button pins
	PORTC |= (1 << INCREMENT_BUTTON) | (1 << DECREMENT_BUTTON) | (1 << CONVERT_BUTTON) | (1 << ALARM_STOP) | (1 << MODE_BUTTON);

	// Set LED pins as output
	DDRB |= (1 << LED1) | (1 << LED5);
	DDRD |= (1 << LED2) | (1 << LED3) | (1 << LED4);

	// SSD binary representations for numbers 0-9 and characters C and F
	uint8_t numbers[] = {
		0b01111110, // 0
		0b00001100, // 1
		0b10110110, // 2
		0b10011110, // 3
		0b11001100, // 4
		0b11011010, // 5
		0b11111010, // 6
		0b00001110, // 7
		0b11111110, // 8
		0b11001110, // 9
		0b01110010, // C
		0b11100010  // F
	};

	// Set default values for the SSD (027C)
	uint8_t digits[] = {0, 2, 7, 10}; // Last digit as 'C' (default Celsius)
	uint8_t isCelsius = 1;

	// Initialize timer
	initTimer();
	// Initialize PWM timer for brightness control
	initPWMTimer2();
	initPWMTimer0();
	initPWMTimer4();
	initPWMTimer1();
	// Enable global interrupts
	sei();

	while (1) {
		// Check for mode switch button press
		if (!(PINC & (1 << MODE_BUTTON))) {
			_delay_ms(20); // Debounce delay
			if (!(PINC & (1 << MODE_BUTTON))) { // Confirm the button is still pressed
				mode++;
				if (mode > 2) mode = 1;

				// Wait for the button to be released
				while (!(PINC & (1 << MODE_BUTTON))) {
					_delay_ms(20);
				}
			}
		}
		
		// Check if the displayed value exceeds the threshold
		if (temperatureValue > CELSIUS_THRESHOLD) {
			if (!ledBlinking) {
				ledBlinking = 1;
				timerCounter = 0;
			}
			} else {
			if (ledBlinking) {
				ledBlinking = 0;
				PORTB &= ~(1 << LED1); // Turn off the LED if the value is below or equal to the threshold
			}
		}

		// Check if the push button connected to PC2 is pressed (disable LED)
		if (!(PINC & (1 << ALARM_STOP))) {
			_delay_ms(20); // Debounce delay
			if (!(PINC & (1 << ALARM_STOP))) { // Confirm the button is still pressed
				if (ledBlinking) {
					ledBlinking = 0;
					ledDisabledByButton = 1;
					PORTB &= ~(1 << LED1); // Turn off the LED
				}
				// Wait for the button to be released
				while (!(PINC & (1 << ALARM_STOP))) {
					_delay_ms(20);
				}
			}
		}


		// Read buttons for increment and decrement operations
		incrementTemperatureValue();
		decrementTemperatureValue();

		if (mode == 1) {
			
			// Turn off all LEDs controlled by PWM
			OCR2B = 0; // LED2
			OCR0B = 0; // LED3
			OCR0A = 0; // LED4
			OCR2A = 0; // LED5
			
			// Display current values on the SSD
			if (isCelsius) {
				// Display Celsius temperatureValue
				if (temperatureValue < 10) {
					digits[0] = 0;
					digits[1] = 0;
					digits[2] = temperatureValue;
					} else if (temperatureValue < 100) {
					digits[0] = 0;
					digits[1] = temperatureValue / 10;
					digits[2] = temperatureValue % 10;
					} else {
					digits[0] = temperatureValue / 100;
					digits[1] = (temperatureValue / 10) % 10;
					digits[2] = temperatureValue % 10;
				}
				} else {
				// Display Fahrenheit temperatureValue
				float fahrenheit = temperatureValue * 9.0 / 5.0 + 32.0;
				uint16_t tempValue = (uint16_t) round(fahrenheit);
				digits[0] = tempValue / 100;
				digits[1] = (tempValue / 10) % 10;
				digits[2] = tempValue % 10;
				digits[3] = 11; // Set the fourth digit to 'F'
			}

			// Display digits on SSD
			for (int i = 0; i < 4; ++i) {
				if (i == 3) {
					// Display unit (C for Celsius, F for Fahrenheit)
					if (isCelsius) {
						displayNumber(4, numbers[10]); // Display 'C'
						} else {
						displayNumber(4, numbers[11]); // Display 'F'
					}
					} else {
					// Display temperature digits
					displayNumber(i + 1, numbers[digits[i]]);
				}
			}
			
			// Check if the convert button (PC3) is pressed
			if (!(PINC & (1 << CONVERT_BUTTON))) {
				_delay_ms(20); // Debounce delay
				if (!(PINC & (1 << CONVERT_BUTTON))) { // Confirm the button is still pressed
					// Toggle the temperature unit
					isCelsius = !isCelsius;

					// Wait for the button to be released
					while (!(PINC & (1 << CONVERT_BUTTON))) {
						_delay_ms(20);
					}
				}
			}

			} else if (mode == 2) {
			
			// Update the brightness of the LEDs
			updateLEDBrightness(temperatureValue, isCelsius);
			updateLEDBrightness3(temperatureValue, isCelsius);
			updateLEDBrightness4(temperatureValue, isCelsius);
			updateLEDBrightness5(temperatureValue, isCelsius);
		}
		
		// Reset LED disabled flag if value is below threshold and LED is not already blinking
		if (isCelsius && temperatureValue <= CELSIUS_THRESHOLD && !ledBlinking) {
			ledDisabledByButton = 0;
		}

	}
	return 0;
}