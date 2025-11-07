
// Structure describing the signature and position of the patch
struct defpatch {
 const char* sig; // signature
 uint32_t sigsize; // signature length
 int32_t poffset;  // offset to the patch point from the end of the signature
};



//***********************************************************************
//* Signature search and patch application
//***********************************************************************
uint32_t patch(struct defpatch fp, uint8_t* buf, uint32_t fsize, uint32_t ptype);

//****************************************************
//* Patching procedures for different chipsets and tasks
//****************************************************

uint32_t pv7r22 (uint8_t* buf, uint32_t fsize);
uint32_t pv7r22_2 (uint8_t* buf, uint32_t fsize);
uint32_t pv7r22_3 (uint8_t* buf, uint32_t fsize);
uint32_t pv7r2 (uint8_t* buf, uint32_t fsize);
uint32_t pv7r11 (uint8_t* buf, uint32_t fsize);
uint32_t pv7r1 (uint8_t* buf, uint32_t fsize);
uint32_t perasebad (uint8_t* buf, uint32_t fsize);

