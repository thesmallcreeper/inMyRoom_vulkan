enum class glTFmode
{
	points = 0,
	line = 1,
	loop = 2,
	line_strip = 3,
	triangles = 4,
	triangle_strip = 5,
	triangle_fan = 6
};

enum class glTFcomponentType
{
	type_byte = 5120,
	type_unsigned_byte = 5121,
	type_short = 5122,
	type_unsigned_short = 5123,
	type_int = 5124,
	type_unsigned_int = 5125,
	type_float = 5126,
	type_double = 5130
};

enum class glTFparameterType
{
	type_byte = 5120,
	type_unsigned_byte = 5121,
	type_short = 5122,
	type_unsigned_short = 5123,
	type_int = 5124,
	type_unsigned_int = 5125,
	type_float = 5126,
	type_double = 5130,

	type_float_vec2 = 35664,
	type_float_vec3 = 35665,
	type_float_vec4 = 35666,

	type_int_vec2 = 35667,
	type_int_vec3 = 35668,
	type_int_vec4 = 35669,

	type_bool = 35670,
	type_bool_vec2 = 35671,
	type_bool_vec3 = 35672,
	type_bool_vec4 = 35673,

	type_float_mat2 = 35674,
	type_float_mat3 = 35675,
	type_float_mat4 = 35676,

	type_sampler_2D = 35678
};

