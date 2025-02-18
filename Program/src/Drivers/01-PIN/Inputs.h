/*!
 * @file Inputs.h
 * @par Author & Doxygen Editor
 * 	Daniel Di Módica ~ <a href = "mailto: danifabriziodmodica@gmail.com">danifabriziodmodica@@gmail.com</a>
 * @date 14/06/2023 20:36:24
 * @brief Inputs Abstract Class API (Application Programming Interface).
 */

#ifndef INPUTS_H_
#define INPUTS_H_

#include "utils.h"

class Inputs {
public:
	virtual void setDirInputs(void) = 0;
	virtual bool getPin(void) const = 0;
	virtual void setPinMode(void) = 0;
	virtual ~Inputs() = default;
};

#endif /* INPUTS_H_ */
