#ifndef __ALFINA_MATH_TYPES_H__
#define __ALFINA_MATH_TYPES_H__

#include <cstdint>
#include <utility>
#include <array>

#include "engine/engine_utilities/asserts.h"

#include "utilities/flags.h"
#include "utilities/constexpr_functions.h"

// @TODO : add simd support
// @TODO : add tests to check correctness of the formulas

namespace al
{
	namespace elements
	{
		constexpr uint8_t X = 0;
		constexpr uint8_t Y = 1;
		constexpr uint8_t Z = 2;
		constexpr uint8_t W = 3;
	
		constexpr uint8_t R = 0;
		constexpr uint8_t G = 1;
		constexpr uint8_t B = 2;
		constexpr uint8_t A = 3;
	}

	// ========================================
	// Vectors
	// ========================================

	template<typename T, size_t num>
	struct mult
	{
		mult() 
			: components{ }
		{ }

		mult(T value)
		{ 
			components.fill(value);
		}

		mult(const std::array<T, num>& array) 
			: components{ array } 
		{ }

		mult(const mult<T, num>& other) 
			: components{ other.components } 
		{ }

		template<class... inT>
		mult(inT... args) 
			: components { static_cast<T>(args)... }
		{ }

		inline T&       get         (size_t element) const  { AL_ASSERT(element < num); return const_cast<T&>(components[element]); }
		inline T&       operator [] (size_t element) const  { AL_ASSERT(element < num); return const_cast<T&>(components[element]); }
		inline size_t   size        ()               const  { return num; }

		T dot(const mult<T, num>& other) const
		{ 
			return dot(other.components); 
		}

		T dot(const std::array<T, num>& other) const
		{ 
			T result = {};
			for (size_t it = 0; it < num; ++it)
				result += components[it] * other[it];
			return result; 
		}

		mult<T, num> cross(const mult<T, num>& other) const
		{ 
			static_assert(num == 3, "cross-product is defined only for 3-component vectors");
			using namespace elements;
			return 
			{
				get(Y) * other.get(Z) - get(Z) * other.get(Y),
				get(Z) * other.get(X) - get(X) * other.get(Z),
				get(X) * other.get(Y) - get(Y) * other.get(X)
			};
		}

		mult<T, num> cross(const std::array<T, num>& other) const
		{
			static_assert(num == 3, "cross-product is defined only for 3-component vectors");
			using namespace elements;
			return 
			{
				get(Y) * other[Z] - get(Z) * other[Y],
				get(Z) * other[X] - get(X) * other[Z],
				get(X) * other[Y] - get(Y) * other[X]
			};
		}

		template<size_t ... indexes>
		mult<T, sizeof ... (indexes)> slice() const
		{
			size_t indexesArray[] = { indexes... };
			auto result = mult<T, sizeof ... (indexes)>();
			for (size_t it = 0; it < sizeof ... (indexes); ++it)
				result[it] = components[indexesArray[it]];
			return result;
		}

		template<typename T, size_t num>
		friend std::ostream& operator << (std::ostream& os, const mult<T, num>& vec);

	protected:
		std::array<T, num> components;
	};

	template<typename T, size_t num>
	std::ostream& operator << (std::ostream& os, const mult<T, num>& vec)
	{
		os << "[ ";
		for (size_t it = 0; it < num; ++it)
			os << vec.components[it] << " ";
		os << "]";
		return os;
	}

	using float2 = mult<float, 2>;
	using float3 = mult<float, 3>;
	using float4 = mult<float, 4>;

	using int32_2 = mult<int32_t, 2>;
	using int32_3 = mult<int32_t, 3>;
	using int32_4 = mult<int32_t, 4>;

	using int64_2 = mult<int64_t, 2>;
	using int64_3 = mult<int64_t, 3>;
	using int64_4 = mult<int64_t, 4>;

	// ========================================
	// Matrix
	// ========================================

	template<typename T, size_t rows, size_t columns>
	class matrix2d
	{
	public:
		matrix2d() 
			: components{ }
		{ }

		matrix2d(T value)
		{ 
			components.fill(value);
		}

		matrix2d(std::array<T, rows * columns> array)
			: components { array }
		{ }

		matrix2d(const matrix2d<T, rows, columns>& other)
			: components { other.components }
		{ }

		template<class... inT>
		matrix2d(inT... args) 
			: components { static_cast<T>(args)... }
		{ }

		inline const T* data_ptr() const
		{
			return &components[0];
		}

		inline T& get(size_t row, size_t column) const
		{
			AL_ASSERT(row < rows)
			AL_ASSERT(column < columns)
			return const_cast<T&>(components[row * columns + column]);
		}

		inline mult<T, columns>& operator [] (size_t row) const
		{ 
			AL_ASSERT(row < rows) 
			return const_cast<mult<T, columns>&>(vec[row]); 
		}

		template<size_t secondColumns>
		matrix2d<T, rows, secondColumns> mul(const matrix2d<T, columns, secondColumns>& other) const
		{
			matrix2d<T, rows, secondColumns> result;

			for (size_t rowsIt = 0; rowsIt < rows; ++rowsIt)
				for (size_t columnsIt = 0; columnsIt < secondColumns; ++columnsIt)
					for (size_t dotIt = 0; dotIt < columns; ++dotIt)
					{
						result.get(rowsIt, columnsIt) += get(rowsIt, dotIt) * other.get(dotIt, columnsIt);
					}

			return result;
		}

		template<typename U>
		matrix2d<T, rows, columns> mul(const U& value) const
		{
			matrix2d<T, rows, columns> result;

			for (size_t rowsIt = 0; rowsIt < rows; ++rowsIt)
				for (size_t columnsIt = 0; columnsIt < columns; ++columnsIt)
				{
					result.get(rowsIt, columnsIt) = get(rowsIt, columnsIt) * value;
				}

			return result;
		}

		mult<T, rows> mul(const mult<T, columns>& vector) const
		{
			mult<T, rows> result;

			for (size_t rowsIt = 0; rowsIt < rows; ++rowsIt)
			{
				T value = static_cast<T>(0);
				for (size_t columnsIt = 0; columnsIt < columns; ++columnsIt)
				{
					value += get(rowsIt, columnsIt) * vector[columnsIt];
				}
				result[rowsIt] = value;
			}

			return result;
		}

		template<size_t secondColumns>
		matrix2d<T, rows, secondColumns> operator * (const matrix2d<T, columns, secondColumns>& other) const
		{
			return mul(other);
		}

		template<typename U>
		matrix2d<T, rows, columns> operator * (const U& value) const
		{
			return mul(value);
		}

		virtual T det() const
		{
			AL_NO_IMPLEMENTATION_ASSERT
			return {};
		}

		virtual matrix2d<T, rows, columns> invert() const
		{
			AL_NO_IMPLEMENTATION_ASSERT
			return {};
		}

		matrix2d<T, columns, rows> transpose() const
		{
			matrix2d<T, columns, rows> result;
			for (size_t rowsIt = 0; rowsIt < rows; ++rowsIt)
				for (size_t columnsIt = 0; columnsIt < columns; ++columnsIt)
				{
					result[columnsIt][rowsIt] = get(rowsIt, columnsIt);
				}
			return result;
		}

		template<typename T, size_t rows, size_t columns>
		friend std::ostream& operator << (std::ostream& os, const matrix2d<T, rows, columns>& mat);

	private:
		union
		{
			std::array<T, columns * rows>               components;
			std::array<std::array<T, columns>, rows>    mat;
			std::array<mult<T, columns>, rows>          vec;
		};
	};

	template<typename T, size_t rows, size_t columns>
	std::ostream& operator << (std::ostream& os, const matrix2d<T, rows, columns>& mat)
	{
		for (size_t rowsIt = 0; rowsIt < rows; ++rowsIt)
		{
			os << "[ ";
			for (size_t columnsIt = 0; columnsIt < columns; ++columnsIt)
			{
				os << mat.get(rowsIt, columnsIt) << " ";
			}
			os << "]\n";
		}

		return os;
	}

	// @TODO : write more unified det() and invert() methods for 2x2, 3x3, 4x4 matricies
	//         p.s. see https://github.com/willnode/N-Matrix-Programmer
	template<typename T>
	class matrix2x2 : public matrix2d<T, 2, 2>
	{
	public:
		using matrix2d::matrix2d;

		virtual T det() const override
		{
			return get(0, 0) * get(1, 1) - get(0, 1) * get(1, 0);
		}

		static inline T det(T a, T b, T c, T d)
		{
			return a * d - b * c;
		}

		// @TODO : instead of using float as calculation type 
		//         add anoter template argument to a class, which 
		//         describes the type of invert() calculation
		//         + add al::floating_point_assert()
		virtual matrix2d<T, 2, 2> invert() const override
		{
			const auto determinant = det();
			// @TODO : add method which returns bool istead of asserting
			AL_ASSERT(!is_equal(determinant, static_cast<T>(0)))
			const auto invDet = 1.0f / determinant;

			return 
			{
						get(1, 1) * invDet, -1.0f * get(0, 1) * invDet,
				-1.0f * get(1, 0) * invDet,         get(0, 0) * invDet
			};
		}
	};

	template<typename T>
	class matrix3x3 : public matrix2d<T, 3, 3>
	{
	public:
		using matrix2d::matrix2d;

		virtual T det() const override
		{
			return 
				get(0, 0) * (get(1, 1) * get(2, 2) - get(1, 2) * get(2, 1)) -
				get(0, 1) * (get(1, 0) * get(2, 2) - get(1, 2) * get(2, 0)) +
				get(0, 2) * (get(1, 0) * get(2, 1) - get(1, 1) * get(2, 0));
		}

		// @TODO : instead of using float as calculation type 
		//         add anoter template argument to a class, which 
		//         describes the type of invert() calculation
		//         + add al::floating_point_assert()
		virtual matrix2d<T, 3, 3> invert() const override
		{
			// this algorithm is taken from https://www.wikihow.com/Find-the-Inverse-of-a-3x3-Matrix

			const auto determinant = det();
			// @TODO : add method which returns bool istead of asserting
			AL_ASSERT(!is_equal(determinant, static_cast<T>(0)))
			const auto invDet = 1.0f / determinant;

			// 2x2 sub-determinants of transposed matrix required to calculate 3x3 inverse
			const T det2_00 = matrix2x2<T>::det(get(1, 1), get(2, 1), get(1, 2), get(2, 2));
			const T det2_01 = matrix2x2<T>::det(get(0, 1), get(2, 1), get(0, 2), get(2, 2));
			const T det2_02 = matrix2x2<T>::det(get(0, 1), get(1, 1), get(0, 2), get(1, 2));
			const T det2_10 = matrix2x2<T>::det(get(1, 0), get(2, 0), get(1, 2), get(2, 2));
			const T det2_11 = matrix2x2<T>::det(get(0, 0), get(2, 0), get(0, 2), get(2, 2));
			const T det2_12 = matrix2x2<T>::det(get(0, 0), get(1, 0), get(0, 2), get(1, 2));
			const T det2_20 = matrix2x2<T>::det(get(1, 0), get(2, 0), get(1, 1), get(2, 1));
			const T det2_21 = matrix2x2<T>::det(get(0, 0), get(2, 0), get(0, 1), get(2, 1));
			const T det2_22 = matrix2x2<T>::det(get(0, 0), get(1, 0), get(1, 0), get(1, 1));

			return 
			{
				invDet         * det2_00, invDet * -1.0f * det2_01, invDet         * det2_02,
				invDet * -1.0f * det2_10, invDet         * det2_11, invDet * -1.0f * det2_12,
				invDet         * det2_20, invDet * -1.0f * det2_21, invDet         * det2_22
			};
		}
	};

	template<typename T>
	class matrix4x4 : public matrix2d<T, 4, 4>
	{
	public:
		using matrix2d::matrix2d;

		virtual T det() const override
		{
			// 2x2 sub-determinants required to calculate 4x4 determinant
			const float det2_01_01 = get(0, 0) * get(1, 1) - get(0, 1) * get(1, 0);
			const float det2_01_02 = get(0, 0) * get(1, 2) - get(0, 2) * get(1, 0);
			const float det2_01_03 = get(0, 0) * get(1, 3) - get(0, 3) * get(1, 0);
			const float det2_01_12 = get(0, 1) * get(1, 2) - get(0, 2) * get(1, 1);
			const float det2_01_13 = get(0, 1) * get(1, 3) - get(0, 3) * get(1, 1);
			const float det2_01_23 = get(0, 2) * get(1, 3) - get(0, 3) * get(1, 2);

			// 3x3 sub-determinants required to calculate 4x4 determinant
			const float det3_201_012 = get(2, 0) * det2_01_12 - get(2, 1) * det2_01_02 + get(2, 2) * det2_01_01;
			const float det3_201_013 = get(2, 0) * det2_01_13 - get(2, 1) * det2_01_03 + get(2, 3) * det2_01_01;
			const float det3_201_023 = get(2, 0) * det2_01_23 - get(2, 2) * det2_01_03 + get(2, 3) * det2_01_02;
			const float det3_201_123 = get(2, 1) * det2_01_23 - get(2, 2) * det2_01_13 + get(2, 3) * det2_01_12;

			return 
				-1.0f * det3_201_123 * get(3, 0) + 
						det3_201_023 * get(3, 1) - 
						det3_201_013 * get(3, 2) + 
						det3_201_012 * get(3, 3);
		}

		// @TODO : instead of using float as calculation type 
		//         add anoter template argument to a class, which 
		//         describes the type of invert() calculation
		//         + add al::floating_point_assert()
		virtual matrix2d<T, 4, 4> invert() const override
		{
			// https://stackoverflow.com/questions/1148309/inverting-a-4x4-matrix
			// https://github.com/willnode/N-Matrix-Programmer

			const auto determinant = det();
			// @TODO : add method which returns bool istead of asserting
			AL_ASSERT(!is_equal(determinant, static_cast<T>(0)))
			const auto invDet = 1.0f / determinant;

			const float A2323 = get(2, 2) * get(3, 3) - get(2, 3) * get(3, 2);
			const float A1323 = get(2, 1) * get(3, 3) - get(2, 3) * get(3, 1);
			const float A1223 = get(2, 1) * get(3, 2) - get(2, 2) * get(3, 1);
			const float A0323 = get(2, 0) * get(3, 3) - get(2, 3) * get(3, 0);
			const float A0223 = get(2, 0) * get(3, 2) - get(2, 2) * get(3, 0);
			const float A0123 = get(2, 0) * get(3, 1) - get(2, 1) * get(3, 0);
			const float A2313 = get(1, 2) * get(3, 3) - get(1, 3) * get(3, 2);
			const float A1313 = get(1, 1) * get(3, 3) - get(1, 3) * get(3, 1);
			const float A1213 = get(1, 1) * get(3, 2) - get(1, 2) * get(3, 1);
			const float A2312 = get(1, 2) * get(2, 3) - get(1, 3) * get(2, 2);
			const float A1312 = get(1, 1) * get(2, 3) - get(1, 3) * get(2, 1);
			const float A1212 = get(1, 1) * get(2, 2) - get(1, 2) * get(2, 1);
			const float A0313 = get(1, 0) * get(3, 3) - get(1, 3) * get(3, 0);
			const float A0213 = get(1, 0) * get(3, 2) - get(1, 2) * get(3, 0);
			const float A0312 = get(1, 0) * get(2, 3) - get(1, 3) * get(2, 0);
			const float A0212 = get(1, 0) * get(2, 2) - get(1, 2) * get(2, 0);
			const float A0113 = get(1, 0) * get(3, 1) - get(1, 1) * get(3, 0);
			const float A0112 = get(1, 0) * get(2, 1) - get(1, 1) * get(2, 0);

			return
			{
			   invDet         * (get(1, 1) * A2323 - get(1, 2) * A1323 + get(1, 3) * A1223),
			   invDet * -1.0f * (get(0, 1) * A2323 - get(0, 2) * A1323 + get(0, 3) * A1223),
			   invDet         * (get(0, 1) * A2313 - get(0, 2) * A1313 + get(0, 3) * A1213),
			   invDet * -1.0f * (get(0, 1) * A2312 - get(0, 2) * A1312 + get(0, 3) * A1212),

			   invDet * -1.0f * (get(1, 0) * A2323 - get(1, 2) * A0323 + get(1, 3) * A0223),
			   invDet         * (get(0, 0) * A2323 - get(0, 2) * A0323 + get(0, 3) * A0223),
			   invDet * -1.0f * (get(0, 0) * A2313 - get(0, 2) * A0313 + get(0, 3) * A0213),
			   invDet         * (get(0, 0) * A2312 - get(0, 2) * A0312 + get(0, 3) * A0212),

			   invDet         * (get(1, 0) * A1323 - get(1, 1) * A0323 + get(1, 3) * A0123),
			   invDet * -1.0f * (get(0, 0) * A1323 - get(0, 1) * A0323 + get(0, 3) * A0123),
			   invDet         * (get(0, 0) * A1313 - get(0, 1) * A0313 + get(0, 3) * A0113),
			   invDet * -1.0f * (get(0, 0) * A1312 - get(0, 1) * A0312 + get(0, 3) * A0112),

			   invDet * -1.0f * (get(1, 0) * A1223 - get(1, 1) * A0223 + get(1, 2) * A0123),
			   invDet         * (get(0, 0) * A1223 - get(0, 1) * A0223 + get(0, 2) * A0123),
			   invDet * -1.0f * (get(0, 0) * A1213 - get(0, 1) * A0213 + get(0, 2) * A0113),
			   invDet         * (get(0, 0) * A1212 - get(0, 1) * A0212 + get(0, 2) * A0112)
			};
		}
	};

	using float2x2 = matrix2x2<float>;
	using float3x3 = matrix3x3<float>;
	using float4x4 = matrix4x4<float>;

	using int64_2x2 = matrix2x2<int64_t>;
	using int64_3x3 = matrix3x3<int64_t>;
	using int64_4x4 = matrix4x4<int64_t>;
}

#endif