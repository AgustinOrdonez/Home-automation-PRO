/*/*!
 * @file M24C16.cpp
 * @par Author & Doxygen Editor
 * 	Daniel Di Módica ~ <a href = "mailto: danifabriziodmodica@gmail.com">danifabriziodmodica@@gmail.com</a>
 * @date 16/10/2023 16:36:21
 */

#include "M24C16.h"

M24C16::pageBlock_t M24C16::m_pageBlock = FIRST_PAGE_BLOCK;

M24C16 *g_eeprom = nullptr;

/****************** EEPROM Testing START ******************
int main(void) {
	initDevice();
	initDisplay();
	initM24C16();

	while (1) {
		if (!(g_eeprom->write(value, position))) LED_RED.ClearPin();
		else LED_RED.SetPin();
		if (!(g_eeprom->read(&currentValue, M24C16::UINT8, position))) LED_BLUE.ClearPin();
		else LED_BLUE.SetPin();
		g_display->set(value, 0);
		g_display->set(currentValue, 1);
		value++; position++; // These are may overflow...
		g_timers_list.TimerEvents();
		delay(1000);
	}
}
******************** EEPROM Testing END *******************/

M24C16::M24C16(const Gpio& SCL, const Gpio& SDA, channelTWI_t channel) : I2C(SCL, SDA, channel),
m_statusEEPROM{EEPROM_OK} { }

/*!
 * @brief Acquire data from the EEPROM memory.
 * @param[out] values Array where the data will be stored.
 * @param numBytes Number of bytes to be acquired.
 * @param position Position where the data will be read from.
 * @param pageBlock EEPROM page where the data will be read from.
 * @return EEPROM status,TWI_SUCCESS if the operation was successful, TWI_FAILURE otherwise.
 */
SyncCommTWI::statusComm_t M24C16::acquire(uint8_t values[], size_t numBytes, uint8_t position, pageBlock_t pageBlock) {
	if (!(this->receiveBytes((M24C16_ADDR_REG | pageBlock), position, values, numBytes)) && !(this->getStatus())) this->m_statusEEPROM = EEPROM_OK;
	else this->m_statusEEPROM = EEPROM_UPDATE_ERR;
	return this->getStatus() ? TWI_FAILURE : TWI_SUCCESS;
}

/*!
 * @brief Transmit data to the EEPROM memory.
 * @param values Array where the data will be stored.
 * @param numBytes Number of bytes to be transmitted.
 * @param position Position where the data will be written to.
 * @param pageBlock EEPROM page where the data will be written to.
 * @return EEPROM status,TWI_SUCCESS if the operation was successful, TWI_FAILURE otherwise.
 */
SyncCommTWI::statusComm_t M24C16::transmit(uint8_t values[], size_t numBytes, uint8_t position, pageBlock_t pageBlock) {
	if (!(this->transmitBytes((M24C16_ADDR_REG | pageBlock), position, values, numBytes)) && !(this->getStatus())) this->m_statusEEPROM = EEPROM_OK;
	else this->m_statusEEPROM = EEPROM_UPDATE_ERR;
	return this->getStatus() ? TWI_FAILURE : TWI_SUCCESS;
}

/*!
 * @brief Read one byte of data from the EEPROM memory.
 * @tparam T Type to be read, only char,uin8t_t and int8_t are permitted.
 * @param[out] data Pointer to where the data will be stored.
 * @param modifier Type of data to be read.
 * @param position Position where the data will be read from.
 * @param pageBlock EEPROM page where the data will be read from.
 * @param middleByte TODO: FINISH THIS
 * @return EEPROM status,EEPROM_OK if the operation was successful.
 */
template <typename T> EEPROM_result_t M24C16::read(T *data, modifierType_t modifier, uint8_t position, pageBlock_t pageBlock, middleByte_t middleByte) {
	this->m_statusEEPROM = EEPROM_OK;

	#if M24C16_USING_MODIFIERS == 1
	byteReg_ut reg;
	if ((position * BYTE_SIZE) + BYTE_SIZE > MAX_PAGE_BLOCK_BITS - (BYTE_SIZE * 2) && (modifier == FLOAT || modifier == INT32 || modifier == UINT32)) {
		this->m_statusEEPROM = EEPROM_OVERFLOW_INVALID;
		return this->getStatus();
	} else if ((position * BYTE_SIZE) + BYTE_SIZE > MAX_PAGE_BLOCK_BITS - BYTE_SIZE) {
		this->m_statusEEPROM = EEPROM_OVERFLOW_INVALID;
		return this->getStatus();
	}

	if (this->m_pageBlock != pageBlock) this->m_pageBlock = pageBlock;
	if (modifier == FLOAT || modifier == INT32 || modifier == UINT32) {
		uint8_t valuesFST[2], valuesSND[2];
		if (this->acquire(valuesFST, sizeof(valuesFST), position, pageBlock)) return this->getStatus();
		if (this->acquire(valuesSND, sizeof(valuesSND), position + 1, pageBlock)) return this->getStatus();
		reg.UInt8[0] = valuesFST[0]; reg.UInt8[1] = valuesFST[1];
		reg.UInt8[2] = valuesSND[0]; reg.UInt8[3] = valuesSND[1];
		if (modifier == FLOAT) *data = reg.Float;
		else if (modifier == INT32) *data = (int32_t)reg.UInt32;
		else if (modifier == UINT32) *data = reg.UInt32;
	} else if (modifier == INT16 || modifier == UINT16) {
		uint8_t values[2];
		if (this->acquire(values, sizeof(values), position, pageBlock)) return this->getStatus();
		reg.UInt8[0] = values[0]; reg.UInt8[1] = values[1];
		if (modifier == INT16) *data = (int16_t)reg.UInt16[0];
		else if (modifier == UINT16) *data = reg.UInt16[0];
	} else if (modifier == INT8 || modifier == UINT8 || modifier == CHAR) {
		uint8_t values[2];
		if (this->acquire(values, sizeof(values), position, pageBlock)) return this->getStatus();
		if (middleByte == FST_QUARTER_BYTE) {
			if (modifier == INT8) *data = (int8_t)values[0];
			else if (modifier == UINT8) *data = values[0];
			else if (modifier == CHAR) *data = (char)values[0];
		} else if (middleByte == SND_QUARTER_BYTE) {
			if (modifier == INT8) *data = (int8_t)values[1];
			else if (modifier == UINT8) *data = values[1];
			else if (modifier == CHAR) *data = (char)values[1];
		}
	} else this->m_statusEEPROM = EEPROM_INCORRECT_MODIFIER;
	#elif M24C16_USING_MODIFIERS == 0
	if ((position * BYTE_SIZE) + BYTE_SIZE > MAX_PAGE_BLOCK_BITS - BYTE_SIZE) {
		this->m_statusEEPROM = EEPROM_OVERFLOW_INVALID;
		return this->getStatus();
	}

	if (this->m_pageBlock != pageBlock) this->m_pageBlock = pageBlock;
	if (modifier == INT8 || modifier == UINT8 || modifier == CHAR) {
		uint8_t values[2];
		if (this->acquire(values, sizeof(values), position, pageBlock)) return this->getStatus();
		if (middleByte == FST_QUARTER_BYTE) {
//			if (modifier == INT8) *data = (int8_t)values[0];
//			else if (modifier == UINT8) *data = values[0];
//			else if (modifier == CHAR) *data = (char)values[0];
            *data = values[0];
		} else if (middleByte == SND_QUARTER_BYTE) {
//			if (modifier == INT8) *data = (int8_t)values[1];
//			else if (modifier == UINT8) *data = values[1];
//			else if (modifier == CHAR) *data = (char)values[1];
            *data = values[1];
		}
	} else this->m_statusEEPROM = EEPROM_INCORRECT_MODIFIER;
	#endif
	return this->getStatus();
}

#if M24C16_USING_MODIFIERS == 1
template EEPROM_result_t M24C16::read<float>(float* data, modifierType_t modifier, uint8_t position, pageBlock_t pageBlock, middleByte_t middleByte);
template EEPROM_result_t M24C16::read<int32_t>(int32_t* data, modifierType_t modifier, uint8_t position, pageBlock_t pageBlock, middleByte_t middleByte);
template EEPROM_result_t M24C16::read<uint32_t>(uint32_t* data, modifierType_t modifier, uint8_t position, pageBlock_t pageBlock, middleByte_t middleByte);
template EEPROM_result_t M24C16::read<int16_t>(int16_t* data, modifierType_t modifier, uint8_t position, pageBlock_t pageBlock, middleByte_t middleByte);
template EEPROM_result_t M24C16::read<uint16_t>(uint16_t* data, modifierType_t modifier, uint8_t position, pageBlock_t pageBlock, middleByte_t middleByte);
template EEPROM_result_t M24C16::read<int8_t>(int8_t* data, modifierType_t modifier, uint8_t position, pageBlock_t pageBlock, middleByte_t middleByte);
template EEPROM_result_t M24C16::read<uint8_t>(uint8_t* data, modifierType_t modifier, uint8_t position, pageBlock_t pageBlock, middleByte_t middleByte);
#elif M24C16_USING_MODIFIERS == 0
template EEPROM_result_t M24C16::read<int8_t>(int8_t* data, modifierType_t modifier, uint8_t position, pageBlock_t pageBlock, middleByte_t middleByte);
template EEPROM_result_t M24C16::read<uint8_t>(uint8_t* data, modifierType_t modifier, uint8_t position, pageBlock_t pageBlock, middleByte_t middleByte);
#endif

/*!
 * @brief Write one byte of data to the EEPROM memory.
 * @tparam T Type to be written, only char,uin8t_t and int8_t are permitted.
 * @param data Data to be written.
 * @param position Position where the data will be written to.
 * @param pageBlock EEPROM page where the data will be written to.
 * @param middleByte TODO:FINISH THIS
 * @return
 */
template <typename T> EEPROM_result_t M24C16::write(T data, uint8_t position, pageBlock_t pageBlock, middleByte_t middleByte) {
	this->m_statusEEPROM = EEPROM_OK;

	#if M24C16_USING_MODIFIERS == 1
	byteReg_ut reg;
	if ((position * BYTE_SIZE) + BYTE_SIZE > MAX_PAGE_BLOCK_BITS - (BYTE_SIZE * 2) && (std::is_same<T, float>::value || std::is_same<T, int32_t>::value || std::is_same<T, uint32_t>::value)) {
		this->m_statusEEPROM = EEPROM_OVERFLOW_INVALID;
		return this->getStatus();
	} else if ((position * BYTE_SIZE) + BYTE_SIZE > MAX_PAGE_BLOCK_BITS - BYTE_SIZE) {
		this->m_statusEEPROM = EEPROM_OVERFLOW_INVALID;
		return this->getStatus();
	}

	if (this->m_pageBlock != pageBlock) this->m_pageBlock = pageBlock;
	if (std::is_same<T, float>::value || std::is_same<T, int32_t>::value || std::is_same<T, uint32_t>::value) {
		if (std::is_same<T, float>::value) reg.Float = data;
		else if (std::is_same<T, int32_t>::value || std::is_same<T, uint32_t>::value) reg.UInt32 = data;
		uint8_t valuesFST[2] = { reg.UInt8[0], reg.UInt8[1] };
		uint8_t valuesSND[2] = { reg.UInt8[2], reg.UInt8[3] };
		if (this->transmit(valuesFST, sizeof(valuesFST), position, pageBlock)) return this->getStatus();
		if (this->transmit(valuesSND, sizeof(valuesSND), position + 1, pageBlock)) return this->getStatus();
	} else if (std::is_same<T, int16_t>::value || std::is_same<T, uint16_t>::value) {
		reg.UInt16[0] = data;
		uint8_t values[2] = { reg.UInt8[0], reg.UInt8[1] };
		if (this->transmit(values, sizeof(values), position, pageBlock)) return this->getStatus();
	} else if (std::is_same<T, int8_t>::value || std::is_same<T, uint8_t>::value) {
		uint8_t values[2];
		if (this->acquire(values, sizeof(values), position, pageBlock)) return this->getStatus();
		if (middleByte == FST_QUARTER_BYTE) {
			values[0] = data;
			if (this->transmit(values, sizeof(values), position, pageBlock)) return this->getStatus();
		} else if (middleByte == SND_QUARTER_BYTE) {
			values[1] = data;
			if (this->transmit(values, sizeof(values), position, pageBlock)) return this->getStatus();
		}
	} else this->m_statusEEPROM = EEPROM_INCORRECT_MODIFIER;
	#elif M24C16_USING_MODIFIERS == 0
	 if ((position * BYTE_SIZE) + BYTE_SIZE > MAX_PAGE_BLOCK_BITS - BYTE_SIZE) {
		this->m_statusEEPROM = EEPROM_OVERFLOW_INVALID;
		return this->getStatus();
	}

	if (this->m_pageBlock != pageBlock) this->m_pageBlock = pageBlock;
	if (std::is_same<T, int8_t>::value || std::is_same<T, uint8_t>::value) {
		uint8_t values[2];
		if (this->acquire(values, sizeof(values), position, pageBlock)) return this->getStatus();
		if (middleByte == FST_QUARTER_BYTE) {
			values[0] = data;
			if (this->transmit(values, sizeof(values), position, pageBlock)) return this->getStatus();
		} else if (middleByte == SND_QUARTER_BYTE) {
			values[1] = data;
			if (this->transmit(values, sizeof(values), position, pageBlock)) return this->getStatus();
		}
	} else this->m_statusEEPROM = EEPROM_INCORRECT_MODIFIER;
	#endif
	return this->getStatus();
}

#if M24C16_USING_MODIFIERS == 1
template EEPROM_result_t M24C16::write<float>(float data, uint8_t position, pageBlock_t pageBlock, middleByte_t middleByte);
template EEPROM_result_t M24C16::write<int32_t>(int32_t data, uint8_t position, pageBlock_t pageBlock, middleByte_t middleByte);
template EEPROM_result_t M24C16::write<uint32_t>(uint32_t data, uint8_t position, pageBlock_t pageBlock, middleByte_t middleByte);
template EEPROM_result_t M24C16::write<int16_t>(int16_t data, uint8_t position, pageBlock_t pageBlock, middleByte_t middleByte);
template EEPROM_result_t M24C16::write<uint16_t>(uint16_t data, uint8_t position, pageBlock_t pageBlock, middleByte_t middleByte);
template EEPROM_result_t M24C16::write<int8_t>(int8_t data, uint8_t position, pageBlock_t pageBlock, middleByte_t middleByte);
template EEPROM_result_t M24C16::write<uint8_t>(uint8_t data, uint8_t position, pageBlock_t pageBlock, middleByte_t middleByte);
#elif M24C16_USING_MODIFIERS == 0
template EEPROM_result_t M24C16::write<int8_t>(int8_t data, uint8_t position, pageBlock_t pageBlock, middleByte_t middleByte);
template EEPROM_result_t M24C16::write<uint8_t>(uint8_t data, uint8_t position, pageBlock_t pageBlock, middleByte_t middleByte);
#endif

/*!
 * @brief Get the EEPROM status.
 * @return EEPROM status.
 * @retval EEPROM_OK if the operation was successful.
 * @retval EEPROM_UPDATE_ERR if no valid data has been acquired or transmitted via I2C communication.
 * @retval EEPROM_PAGE_BLOCK_INVALID if the page block exceeds the limit of allowed pages block.
 * @retval EEPROM_OVERFLOW_INVALID if the bytes exceed the limit of allowed bytes into the desired page block.
 * @retval EEPROM_INCORRECT_MODIFIER if the method parameter does not match the supported modifiers.
 */
EEPROM_result_t M24C16::getStatus(void) const {
	return this->m_statusEEPROM;
}

///////////////////////////////
/// FM24C16U initialization ///
///////////////////////////////

void initM24C16(void) {
	#if defined(I2C0_PINS)

	static M24C16 eeprom(I2C0_SCL, I2C0_SDA, I2C::TWI0);

	g_eeprom = &eeprom;

	#endif // defined(I2C0_PINS)
}
