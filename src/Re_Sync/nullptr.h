#pragma once

// GCC поддерживает nullptr начиная с версии 4.6
#if (defined _MSC_VER && _MSC_VER < 1600) || (defined __GNUC__ && (__GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 6)))
# ifdef USE_NULLPTR_T
const class nullptr_t
{
public:
	template<class T>
	inline operator T*() const // convertible to any type of null non-member pointer...
	{ return 0; }

	template<class C, class T>
	inline operator T C::*() const   // or any type of null member pointer...
	{ return 0; }

private:
	void operator&() const;  // Can't take address of nullptr

} nullptr = {};
# else
#  define nullptr NULL
# endif
#endif
