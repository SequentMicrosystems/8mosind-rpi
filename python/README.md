
[![8mosind-rpi](res/sequent.jpg)](https://sequentmicrosystems.com)

# lib8mosind

This is the python library to control the [8-MOSFETS V3 Solid State Stackable Card for Raspberry Pi](https://sequentmicrosystems.com/collections/industrial-automation/products/eight-mosfets-v3-br-8-layer-stackable-card-br-for-raspberry-pi).

## Install

```bash
sudo pip install SM8mosind
```

## Usage

Now you can import the lib8mosind library and use its functions. To test, read mosfets status from the board with stack level 0:

```bash
~$ python
Python 2.7.9 (default, Sep 17 2016, 20:26:04)
[GCC 4.9.2] on linux2
Type "help", "copyright", "credits" or "license" for more information.
>>> import lib8mosind
>>> lib8mosind.get_all(0)
0
>>>
```

## Functions

### set(stack, mosfet, value)
Set one mosfet state.

stack - stack level of the 8-Relay card (selectable from address jumpers [0..7])

mosfet - mosfet number (id) [1..8]

value - mosfet state 1: turn ON, 0: turn OFF[0..1]


### set_all(stack, value)
Set all mosfets state.

stack - stack level of the 8-Relay card (selectable from address jumpers [0..7])

value - 4 bit value of all mosfets (ex: 15: turn on all mosfets, 0: turn off all mosfets, 1:turn on mosfet #1 and off the rest)

### get(stack, mosfet)
Get one mosfet state.

stack - stack level of the 8-Relay card (selectable from address jumpers [0..7])

mosfet - mosfet number (id) [1..8]

return 0 == mosfet off; 1 - mosfet on

### get_all(stack)
Return the state of all mosfets.

stack - stack level of the 8-Relay card (selectable from address jumpers [0..7])

return - [0..255]

# lib8mosind

This is the python library to control the [8mosind](https://sequentmicrosystems.com/index.php?route=product/product&path=33&product_id=69) 8-MOSFETS Stackable Card for Raspberry Pi.

## Install

```bash
sudo pip install SM8mosind
```

## Usage

Now you can import the megaio library and use its functions. To test, read mosfets status from the board with stack level 0:

```bash
~$ python
Python 2.7.9 (default, Sep 17 2016, 20:26:04)
[GCC 4.9.2] on linux2
Type "help", "copyright", "credits" or "license" for more information.
>>> import lib8mosind
>>> lib8mosind.get_all(0)
0
>>>
```

## Functions

### set(stack, mosfet, value)
Set one mosfet state.

stack - stack level of the 8-Relay card (selectable from address jumpers [0..7])

mosfet - mosfet number (id) [1..8]

value - mosfet state 1: turn ON, 0: turn OFF[0..1]


### set_all(stack, value)
Set all mosfets state.

stack - stack level of the 8-Relay card (selectable from address jumpers [0..7])

value - 4 bit value of all mosfets (ex: 15: turn on all mosfets, 0: turn off all mosfets, 1:turn on mosfet #1 and off the rest)

### get(stack, mosfet)
Get one mosfet state.

stack - stack level of the 8-Relay card (selectable from address jumpers [0..7])

mosfet - mosfet number (id) [1..8]

return 0 == mosfet off; 1 - mosfet on

### get_all(stack)
Return the state of all mosfets.

stack - stack level of the 8-Relay card (selectable from address jumpers [0..7])

return - [0..255]
