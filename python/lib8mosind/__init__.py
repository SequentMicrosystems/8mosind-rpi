import smbus

# bus = smbus.SMBus(1)    # 0 = /dev/i2c-0 (port I2C0), 1 = /dev/i2c-1 (port I2C1)

DEVICE_ADDRESS = 0x38  # 7 bit address (will be left shifted to add the read write bit)
ALTERNATE_DEVICE_ADDRESS = 0x20  # 7 bit address (will be left shifted to add the read write bit)
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


def check(bus, add):
    cfg = bus.read_byte_data(add, MOSFET8_CFG_REG_ADD)
    if cfg != 0:
        bus.write_byte_data(add, MOSFET8_CFG_REG_ADD, 0)
        bus.write_byte_data(add, MOSFET8_OUTPORT_REG_ADD, 0xff)
    return bus.read_byte_data(add, MOSFET8_INPORT_REG_ADD)


def check_a(bus, stack):
    global devAdd
    if stack < 0 or stack > 7:
        raise ValueError('Invalid stack level')
    stack = 0x07 ^ stack
    devAdd = DEVICE_ADDRESS + stack
    ret = 0
    try:
        ret = check(bus, devAdd)
    except Exception as e:
        devAdd = ALTERNATE_DEVICE_ADDRESS + stack
        try:
            ret = check(bus, devAdd)
        except Exception as e:
            bus.close()
            raise ValueError('8-mosfets card not detected!')
    return ret


def writeOutput(bus, val):
    global devAdd
    bus.write_byte_data(devAdd, MOSFET8_OUTPORT_REG_ADD, val)


def set(stack, mosfet, value):
    if mosfet < 1:
        raise ValueError('Invalid mosfet number')
    if mosfet > 8:
        raise ValueError('Invalid mosfet number')
    try:
        bus = smbus.SMBus(1)
        oldVal = check_a(bus, stack)
        oldVal = IOToMosfet(oldVal)
        if value == 0:
            oldVal = oldVal & (~(1 << (mosfet - 1)))
            oldVal = mosfetToIO(oldVal)
            writeOutput(bus, oldVal)
        else:
            oldVal = oldVal | (1 << (mosfet - 1))
            oldVal = mosfetToIO(oldVal)
            writeOutput(bus, oldVal)
    except Exception as e:
        bus.close()
        raise ValueError('8-mosfets card not detected!')
    bus.close()


def set_all(stack, value):
    if value > 255:
        raise ValueError('Invalid mosfet value')
    if value < 0:
        raise ValueError('Invalid mosfet value')
    bus = smbus.SMBus(1)
    oldVal = check_a(bus, stack)
    value = mosfetToIO(value)
    try:
        writeOutput(bus, value)
    except Exception as e:
        bus.close()
        raise ValueError('8-mosfets card not detected!')
    bus.close()


def get(stack, mosfet):
    if mosfet < 1:
        raise ValueError('Invalid mosfet number')
    if mosfet > 8:
        raise ValueError('Invalid mosfet number')
    bus = smbus.SMBus(1)
    val = check_a(bus, stack)
    val = IOToMosfet(val)
    val = val & (1 << (mosfet - 1))
    bus.close()
    if val == 0:
        return 0
    else:
        return 1


def get_all(stack):
    bus = smbus.SMBus(1)
    val = check_a(bus, stack)
    val = IOToMosfet(val)
    bus.close()
    return val