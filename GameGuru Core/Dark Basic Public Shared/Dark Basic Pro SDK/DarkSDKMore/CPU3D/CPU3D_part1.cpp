void CameraBuildMatrix(CAMERA_PTR cam, int cam_rot_seq)
{
	MATRIX4X4 mt_inv,  // inverse camera translation matrix
			  mx_inv,  // inverse camera x axis rotation matrix
			  my_inv,  // inverse camera y axis rotation matrix
			  mz_inv,  // inverse camera z axis rotation matrix
			  mrot,    // concatenated inverse rotation matrices
			  mtmp;    // temporary working matrix


	// step 1: create the inverse translation matrix for the camera
	// position
	MatInit4X4(&mt_inv, 1,    0,     0,     0,
						  0,    1,     0,     0,
						  0,    0,     1,     0,
						  -cam->pos.x, -cam->pos.y, -cam->pos.z, 1);

	// step 2: create the inverse rotation sequence for the camera
	// rember either the transpose of the normal rotation matrix or
	// plugging negative values into each of the rotations will result
	// in an inverse matrix

	// first compute all 3 rotation matrices

	// extract out euler angles
	float theta_x = cam->dir.x;
	float theta_y = cam->dir.y;
	float theta_z = cam->dir.z;

	// compute the sine and cosine of the angle x
	float cos_theta = FastCos(theta_x);  // no change since cos(-x) = cos(x)
	float sin_theta = -FastSin(theta_x); // sin(-x) = -sin(x)

	// set the matrix up 
	MatInit4X4(&mx_inv, 1,    0,         0,         0,
						  0,    cos_theta, sin_theta, 0,
						  0,   -sin_theta, cos_theta, 0,
						  0,    0,         0,         1);

	// compute the sine and cosine of the angle y
	cos_theta = FastCos(theta_y);  // no change since cos(-x) = cos(x)
	sin_theta = -FastSin(theta_y); // sin(-x) = -sin(x)

	// set the matrix up 
	MatInit4X4(&my_inv,cos_theta, 0, -sin_theta, 0,  
						 0,         1,  0,         0,
						 sin_theta, 0,  cos_theta,  0,
						 0,         0,  0,          1);

	// compute the sine and cosine of the angle z
	cos_theta = FastCos(theta_z);  // no change since cos(-x) = cos(x)
	sin_theta = -FastSin(theta_z); // sin(-x) = -sin(x)

	// set the matrix up 
	MatInit4X4(&mz_inv, cos_theta, sin_theta, 0, 0,  
						 -sin_theta, cos_theta, 0, 0,
						  0,         0,         1, 0,
						  0,         0,         0, 1);

	// now compute inverse camera rotation sequence
	switch(cam_rot_seq)
	{
		case CAM_ROT_SEQ_XYZ:
		{
			MatMul4X4(&mx_inv, &my_inv, &mtmp);
			MatMul4X4(&mtmp, &mz_inv, &mrot);
		} break;

		case CAM_ROT_SEQ_YXZ:
		{
			MatMul4X4(&my_inv, &mx_inv, &mtmp);
			MatMul4X4(&mtmp, &mz_inv, &mrot);
		} break;

		case CAM_ROT_SEQ_XZY:
		{
			MatMul4X4(&mx_inv, &mz_inv, &mtmp);
			MatMul4X4(&mtmp, &my_inv, &mrot);
		} break;

		case CAM_ROT_SEQ_YZX:
		{
			MatMul4X4(&my_inv, &mz_inv, &mtmp);
			MatMul4X4(&mtmp, &mx_inv, &mrot);
		} break;

		case CAM_ROT_SEQ_ZYX:
		{
			MatMul4X4(&mz_inv, &my_inv, &mtmp);
			MatMul4X4(&mtmp, &mx_inv, &mrot);
		} break;

		case CAM_ROT_SEQ_ZXY:
		{
			MatMul4X4(&mz_inv, &mx_inv, &mtmp);
			MatMul4X4(&mtmp, &my_inv, &mrot);

		} break;

		default: break;
	} 

	// now mrot holds the concatenated product of inverse rotation matrices
	// multiply the inverse translation matrix against it and store in the 
	// camera objects' camera transform matrix
	MatMul4X4(&mt_inv, &mrot, &cam->mcam);

} 

void MatInit4X4(MATRIX4X4_PTR ma, 
                 float m00, float m01, float m02, float m03,
                 float m10, float m11, float m12, float m13,
                 float m20, float m21, float m22, float m23,
                 float m30, float m31, float m32, float m33)

{
	// this function fills a 4x4 matrix with the sent data in 
	// row major form
	ma->M00 = m00; ma->M01 = m01; ma->M02 = m02; ma->M03 = m03;
	ma->M10 = m10; ma->M11 = m11; ma->M12 = m12; ma->M13 = m13;
	ma->M20 = m20; ma->M21 = m21; ma->M22 = m22; ma->M23 = m23;
	ma->M30 = m30; ma->M31 = m31; ma->M32 = m32; ma->M33 = m33;

} 

float FastSin(float theta)
{
	// convert angle to 0-359
	theta = fmodf(theta,360);

	// make angle positive
	if (theta < 0) theta+=360.0;

	// compute floor of theta and fractional part to interpolate
	int theta_int    = (int)theta;
	float theta_frac = theta - theta_int;

	// now compute the value of sin(angle) using the lookup tables
	// and interpolating the fractional part, note that if theta_int
	// is equal to 359 then theta_int+1=360, but this is fine since the
	// table was made with the entries 0-360 inclusive
	return(sin_look[theta_int] + 
		   theta_frac*(sin_look[theta_int+1] - sin_look[theta_int]));

} 

///////////////////////////////////////////////////////////////

float FastCos(float theta)
{

	// convert angle to 0-359
	theta = fmodf(theta,360);

	// make angle positive
	if (theta < 0) theta+=360.0;

	// compute floor of theta and fractional part to interpolate
	int theta_int    = (int)theta;
	float theta_frac = theta - theta_int;

	// now compute the value of sin(angle) using the lookup tables
	// and interpolating the fractional part, note that if theta_int
	// is equal to 359 then theta_int+1=360, but this is fine since the
	// table was made with the entries 0-360 inclusive
	return(cos_look[theta_int] + 
		   theta_frac*(cos_look[theta_int+1] - cos_look[theta_int]));

}

void MatMul4X4(MATRIX4X4_PTR ma, 
                 MATRIX4X4_PTR mb,
                 MATRIX4X4_PTR mprod)
{

	for (int row=0; row<4; row++)
	{
		for (int col=0; col<4; col++)
		{
			// compute dot product from row of ma 
			// and column of mb

			float sum = 0; // used to hold result

			for (int index=0; index<4; index++)
			{
				 // add in next product pair
				 sum+=(ma->M[row][index]*mb->M[index][col]);
			} 

			// insert resulting row,col element
			mprod->M[row][col] = sum;

		} 

	} 

} 

//=======================================================================================
//=======================================================================================
