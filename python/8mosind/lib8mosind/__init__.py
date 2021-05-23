import smbus

# bus = smbus.SMBus(1)    # 0 = /dev/i2c-0 (port I2C0), 1 = /dev/i2c-1 (port I2C1)

DEVICE_ADDRESS = 0x38  # 7 bit address (will be left shifted to add the read write bit)

MOSFET8_INPORT_REG_ADD = 0x00
MOSFET8_OUTPORT_REG_ADD = 0x01
MOSFET8_POLINV_REG_ADD = 0x02
MOSFET8_CFG_REG_ADD = 0x03

mosfetMaskRemap = [0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80]
mosfetChRemap = [0, 1, 2, 3, 4, 5, 6, 7]


def mosfetToIO(mosfet):
    val = 0
    for i in range(0, 8):
        if (mosfet & (1 << i)) != 0:
            val = val + mosfetMaskRemap[i]
    return val ^ 0xff


def IOToMosfet(iov):
    val = 0
    iov = iov ^ 0xff
    for i in range(0, 8):
        if (iov & mosfetMaskRemap[i]) != 0:
            val = val + (1 << i)
    return val


def check(bus, add):
    cfg = bus.read_byte_data(add, MOSFET8_CFG_REG_ADD)
    if cfg != 0:
        bus.write_byte_data(add, MOSFET8_CFG_REG_ADD, 0)
        bus.write_byte_data(add, MOSFET8_OUTPORT_REG_ADD, 0xff)
    return bus.read_byte_data(add, MOSFET8_INPORT_REG_ADD)


def set(stack, mosfet, value):
    if stack < 0 or stack > 7:
        raise ValueError('Invalid stack level')
    stack = 0x07 ^ stack
    if mosfet < 1:
        raise ValueError('Invalid mosfet number')
    if mosfet > 8:
        raise ValueError('Invalid mosfet number')
    try:
        bus = smbus.SMBus(1)
        oldVal = check(bus, DEVICE_ADDRESS + stack)
        oldVal = IOToMosfet(oldVal)
        if value == 0:
            oldVal = oldVal & (~(1 << (mosfet - 1)))
            oldVal = mosfetToIO(oldVal)
            bus.write_byte_data(DEVICE_ADDRESS + stack, MOSFET8_OUTPORT_REG_ADD, oldVal)
        else:
            oldVal = oldVal | (1 << (mosfet - 1))
            oldVal = mosfetToIO(oldVal)
            bus.write_byte_data(DEVICE_ADDRESS + stack, MOSFET8_OUTPORT_REG_ADD, oldVal)
    except Exception as e:
        bus.close()
        raise ValueError('8-mosfets card not detected!')
    bus.close()


def set_all(stack, value):
    if stack < 0 or stack > 7:
        raise ValueError('Invalid stack level')
    stack = 0x07 ^ stack
    if value > 255:
        raise ValueError('Invalid mosfet value')
    if value < 0:
        raise ValueError('Invalid mosfet value')
    bus = smbus.SMBus(1)
    try:
        oldVal = check(bus, DEVICE_ADDRESS + stack)
        value = mosfetToIO(value)
        bus.write_byte_data(DEVICE_ADDRESS + stack, MOSFET8_OUTPORT_REG_ADD, value)
    except Exception as e:
        bus.close()
        raise ValueError('8-mosfets card not detected!')
    bus.close()


def get(stack, mosfet):
    if stack < 0 or stack > 7:
        raise ValueError('Invalid stack level')
    stack = 0x07 ^ stack
    if mosfet < 1:
        raise ValueError('Invalid mosfet number')
    if mosfet > 8:
        raise ValueError('Invalid mosfet number')
    try:
        bus = smbus.SMBus(1)
        val = check(bus, DEVICE_ADDRESS + stack)
        val = IOToMosfet(val)
        val = val & (1 << (mosfet - 1))
        bus.close()
        if val == 0:
            return 0
        else:
            return 1
    except Exception as e:
        bus.close()
        raise ValueError('8-mosfets card not detected!')


def get_all(stack):
    if stack < 0 or stack > 7:
        raise ValueError('Invalid stack level')
    stack = 0x07 ^ stack
    try:
        bus = smbus.SMBus(1)
        val = check(bus, DEVICE_ADDRESS + stack)
        val = IOToMosfet(val)
        bus.close()
        return val
    except Exception as e:
        bus.close()
        raise ValueError('8-mosfets card not detected!')
