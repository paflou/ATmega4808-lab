# ATmega4808-lab
A compilation of different lab exercises meant to run on an ATmega4808, as part of the curriculum at the University of Patras.

## 1: AVR Elevator Button Control
This project demonstrates an interrupt-driven button control system on an ATmega4808 microcontroller. It simulates a basic 3-floor elevator LED indicator using external interrupts on Port F buttons and output control on Port D LEDs.

- **LEDs**: Connected to `PORTD` pins **0**, **1**, and **2**
- **Buttons**:
  - `PF5` - Down button
  - `PF6` - Up button

## 🚦 Functionality

- **Three-floor representation** using 3 LEDs:
  - `LED0`: Ground floor
  - `LED1`: First floor
  - `LED2`: Second floor
- **Button-triggered movement**:
  - Press **Down (PF5)** to move down one floor.
  - Press **Up (PF6)** to move up one floor.
- **Interrupts**:
  - Configured on **rising edge** (button release).
  - Enabled with internal pull-up resistors.
- **Error Handling**:
  - If **both buttons are pressed simultaneously**, `LED0` blinks briefly as an error signal.

## 💡 Behavior

- All LEDs start OFF (output high).
- Pressing **Down**:
  - If not on the ground floor:
    - Turns OFF one or more LEDs to move down.
- Pressing **Up**:
  - If not on the second floor:
    - Turns ON one or more LEDs to move up.
- Pressing **both buttons**:
  - Blinks `LED0` to indicate an invalid operation.

## 2: Traffic Light System with Pedestrian and Tram Simulation

This project implements a basic traffic light control system on the ATmega4808 using the **SPLIT timer mode** of the **TCA0** timer. It simulates interaction between **cars**, **pedestrians**, and a **tram**, handling timed light changes and user button interrupts.

### 🚥 System Overview

- **LED Assignments (PORTD)**:
  - `PD0`: Car LED (Green = 0, Red = 1)
  - `PD1`: Tram simulation LED
  - `PD2`: Pedestrian LED (Green = 0, Red = 1)

- **Button Input**:
  - `PF5`: Pedestrian button with pull-up enabled and rising edge interrupt (button release detection)

- **Timers**:
  - **T1** (`HCNT`): Tram duration (~13ms)
  - **T2** (`LCNT`): Pedestrian crossing duration (~6.56ms)
  - **T3**: Button reactivation delay (~1.64ms)

### ⏱ Timing & Operation

- **Prescaler**: 1024  
  - Timer frequency: `20 MHz / 1024 ≈ 19.531 kHz`
  - Timer resolution: 1 count ≈ 51.2 μs

### 🔄 Behavior

1. **Startup**:
   - Car light is **green**, others are **red**.
   - Pedestrian button interrupt is enabled.

2. **Pedestrian presses the button** (`ISR(PORTF_PORT_vect)`):
   - Car light turns **red**, pedestrian light turns **green**.
   - Starts timer `LCNT` for pedestrian crossing duration (`T2`).
   - Disables button interrupt until timer finishes.

3. **Tram event** (`ISR(TCA0_HUNF_vect)`):
   - Car light turns **red**, tram and pedestrian lights turn **green**.
   - Starts pedestrian timer (`LCNT`) and disables button until finished.

4. **Pedestrian crossing ends / button timer ends** (`ISR(TCA0_LUNF_vect)`):
   - If `state == 0`:
     - Turn pedestrian and tram LEDs **red**, car light **green**.
     - Starts reactivation timer (`T3`), sets `state = 1`.
   - If `state == 1`:
     - Re-enables pedestrian button interrupt.
     - Stops the timer, resets `state = 0`.

### 🧠 Highlights

- Demonstrates effective **interrupt-driven** design.
- Uses **split timer mode** to manage multiple independent timeouts.
- Prevents **button re-entry** during light changes using a state machine and delayed reactivation.

## 3: Autonomous Navigation with Obstacle Detection

This lab simulates an autonomous vehicle navigating a grid using ADC-based distance sensors and timers. The robot can **move forward**, **detect obstacles**, **turn accordingly**, and **return to the start position** when a button is pressed.

### 🤖 System Components

- **Sensors (ADC Inputs)**:
  - Front sensor: `AIN6`
  - Right sensor: `AIN7`
  - Left sensor (for returning): `AIN5`
- **LEDs (PORTD 0–2)**:
  - Indicate movement and turning (front, left, right)
- **Button (PF5)**:
  - Initiates return to starting point `(0,0)`

### 🔁 Logic

- Movement is tracked using `x` and `y` coordinates.
- The robot moves forward unless:
  - There's a wall in front → turns **left**.
  - There's no wall to the right → turns **right**.
- Button press:
  - Flips a `returning` flag.
  - Reverses direction (180° turn).
- The robot navigates back using mirrored logic.
- ADC interrupts handle obstacle detection.
- Timers (`T1`, `T2`, `T3`) manage update rates and delays.

### 💡 Highlights

- Implements **directional logic** using a compass-style state machine.
- Makes use of **window comparator ADC interrupts** for real-time wall detection.
- Simulates **autonomous return pathfinding** using the same logic in reverse.
- Utilizes **freerunning mode** and **split timers** for precise control.
