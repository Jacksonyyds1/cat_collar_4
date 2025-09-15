
// /* Definition of I2C address of MLX90632 */
// #define CHIP_ADDRESS 0x3a << 1
// /* HAL_I2C_Mem_Read()/Write() are used instead of Master_Transmit()/Receive() because repeated start condition is needed */
// /* Implementation of I2C read for 16-bit values */
// int32_t mlx90632_i2c_read(int16_t register_address, uint16_t *value)
// {
// 	uint8_t data[2];
// 	int32_t ret;
// 	ret = HAL_I2C_Mem_Read(&hi2c1, CHIP_ADDRESS, register_address, 2, data, sizeof(data), 100);
// 	//Endianness
// 	*value = data[1]|(data[0]<<8);
// 	return ret;
// }

// /* Implementation of I2C read for 32-bit values */
// int32_t mlx90632_i2c_read32(int16_t register_address, uint32_t *value)
// {
// 	uint8_t data[4];
// 	int32_t ret;
// 	ret = HAL_I2C_Mem_Read(&hi2c1, CHIP_ADDRESS, register_address, 2, data, sizeof(data), 100);
// 	//Endianness
// 	*value = data[2]<<24|data[3]<<16|data[0]<<8|data[1];
// 	return ret;
// }

// /* Implementation of I2C write for 16-bit values */
// int32_t mlx90632_i2c_write(int16_t register_address, uint16_t value) {
// 	uint8_t data[2];
// 	data[0] = value >> 8;
// 	data[1] = value;
// 	return HAL_I2C_Mem_Write(&hi2c1, CHIP_ADDRESS, register_address, 2, data, 2, 100);
// }

// /* Implementation of reading all calibration parameters for calucation of Ta and To */
// static int mlx90632_read_eeprom(int32_t *PR, int32_t *PG, int32_t *PO, int32_t *PT, int32_t *Ea, int32_t *Eb, int32_t *Fa, int32_t *Fb, int32_t *Ga, int16_t *Gb, int16_t *Ha, int16_t *Hb, int16_t *Ka)
// {
// 	int32_t ret;
// 	ret = mlx90632_i2c_read32(MLX90632_EE_P_R, (uint32_t *) PR);
// 	if(ret < 0)
// 		return ret;
// 	ret = mlx90632_i2c_read32(MLX90632_EE_P_G, (uint32_t *) PG);
// 	if(ret < 0)
// 		return ret;
// 	ret = mlx90632_i2c_read32(MLX90632_EE_P_O, (uint32_t *) PO);
// 	if(ret < 0)
// 		return ret;
// 	ret = mlx90632_i2c_read32(MLX90632_EE_P_T, (uint32_t *) PT);
// 	if(ret < 0)
// 		return ret;
// 	ret = mlx90632_i2c_read32(MLX90632_EE_Ea, (uint32_t *) Ea);
// 	if(ret < 0)
// 		return ret;
// 	ret = mlx90632_i2c_read32(MLX90632_EE_Eb, (uint32_t *) Eb);
// 	if(ret < 0)
// 		return ret;
// 	ret = mlx90632_i2c_read32(MLX90632_EE_Fa, (uint32_t *) Fa);
// 	if(ret < 0)
// 		return ret;
// 	ret = mlx90632_i2c_read32(MLX90632_EE_Fb, (uint32_t *) Fb);
// 	if(ret < 0)
// 		return ret;
// 	ret = mlx90632_i2c_read32(MLX90632_EE_Ga, (uint32_t *) Ga);
// 	if(ret < 0)
// 		return ret;
// 	ret = mlx90632_i2c_read(MLX90632_EE_Gb, (uint16_t *) Gb);
// 	if(ret < 0)
// 		return ret;
// 	ret = mlx90632_i2c_read(MLX90632_EE_Ha, (uint16_t *) Ha);
// 	if(ret < 0)
// 		return ret;
// 	ret = mlx90632_i2c_read(MLX90632_EE_Hb, (uint16_t *) Hb);
// 	if(ret < 0)
// 		return ret;
// 	ret = mlx90632_i2c_read(MLX90632_EE_Ka, (uint16_t *) Ka);
// 	if(ret < 0)
// 		return ret;
// 	return 0;
// }

// void usleep(int min_range, int max_range) {
// 	while(--min_range);
// }

// double pre_ambient, pre_object, ambient, object;

// int main(void)
// {

// 	/* Check the internal version and prepare a clean start */
// 	mlx90632_init();

// 	/* Definition of MLX90632 calibration parameters */
// 	int16_t ambient_new_raw;
// 	int16_t ambient_old_raw;
// 	int16_t object_new_raw;
// 	int16_t object_old_raw;
// 	int32_t PR = 0x00587f5b;
// 	int32_t PG = 0x04a10289;
// 	int32_t PT = 0xfff966f8;
// 	int32_t PO = 0x00001e0f;
// 	int32_t Ea = 4859535;
// 	int32_t Eb = 5686508;
// 	int32_t Fa = 53855361;
// 	int32_t Fb = 42874149;
// 	int32_t Ga = -14556410;
// 	int16_t Ha = 16384;
// 	int16_t Hb = 0;
// 	int16_t Gb = 9728;
// 	int16_t Ka = 10752;

// 	/* Read EEPROM calibration parameters */
// 	mlx90632_read_eeprom(&PR, &PG, &PO, &PT, &Ea, &Eb, &Fa, &Fb, &Ga, &Gb, &Ha, &Hb, &Ka);
// 	while (1)
// 	{
// 		/* Get raw data from MLX90632 */
// 		mlx90632_read_temp_raw(&ambient_new_raw, &ambient_old_raw, &object_new_raw, &object_old_raw);
// 		/* Pre-calculations for ambient and object temperature calculation */
// 		pre_ambient = mlx90632_preprocess_temp_ambient(ambient_new_raw, ambient_old_raw, Gb);
// 		pre_object = mlx90632_preprocess_temp_object(object_new_raw, object_old_raw, ambient_new_raw, ambient_old_raw, Ka);
// 		/* Set emissivity = 1 */
// 		mlx90632_set_emissivity(1.0);
// 		/* Calculate ambient and object temperature */
// 		ambient = mlx90632_calc_temp_ambient(ambient_new_raw, ambient_old_raw, PT, PR, PG, PO, Gb);
// 		object = mlx90632_calc_temp_object(pre_object, pre_ambient, Ea, Eb, Ga, Fa, Fb, Ha, Hb);
// 	}
// }
