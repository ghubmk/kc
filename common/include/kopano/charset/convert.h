/*
 * SPDX-License-Identifier: AGPL-3.0-only
 * Copyright 2005 - 2016 Zarafa and its licensors
 */
#pragma once
#include <kopano/zcdefs.h>
#include <list>
#include <map>
#include <string>
#include <stdexcept>
#include <iconv.h>
#include <kopano/charset/traits.h>
#include <kopano/charset/utf8string.h>
#include <mapidefs.h>

namespace KC {

/**
 * @brief	Exception class
 */
class KC_EXPORT_THROW convert_exception : public std::runtime_error {
public:
	enum exception_type {
		eUnknownCharset,
		eIllegalSequence
	};
	convert_exception(enum exception_type type, const std::string &message);

	enum exception_type type() const {
		return m_type;
	}

private:
	enum exception_type m_type;
};

/**
 * @brief	Unknown charset
 */
class KC_EXPORT_THROW unknown_charset_exception KC_FINAL :
    public convert_exception {
	public:
	unknown_charset_exception(const std::string &message);
};

/**
 * @brief	Illegal sequence
 */
class KC_EXPORT_THROW illegal_sequence_exception KC_FINAL :
    public convert_exception {
	public:
	illegal_sequence_exception(const std::string &message);
};

/**
 * @brief	Default converter from one charset to another with string types.
 */
class KC_EXPORT iconv_context KC_FINAL {
	public:
	iconv_context(iconv_context &&);
	iconv_context(const char *tocode, const char *fromcode);
	~iconv_context();

	/**
	 * @brief Performs the conversion.
	 *
	 * The actual conversion in delegated to iconv_context_base.
	 * @param[in] lpRaw		Raw pointer to the data to be converted.
	 * @param[in] cbRaw		The size in bytes of the data to be converted.
	 * @return				The converted string.
	 */
	template<typename To_Type> To_Type convert(To_Type *null, const char *lpRaw, size_t cbRaw)
	{
		To_Type m_to;
		doconvert(lpRaw, cbRaw, &m_to, [](void *obj, const char *b, size_t z) {
			static_cast<To_Type *>(obj)->
				append(reinterpret_cast<typename To_Type::const_pointer>(b),
				z / sizeof(typename To_Type::value_type));
		});
		return m_to;
	}

	/**
	 * @brief Performs the conversion.
	 *
	 * The actual conversion in delegated to iconv_context_base.
	 * @param[in] _from		The string to be converted.
	 * @return				The converted string.
	 */
	template<typename To_Type, typename From_Type>
	To_Type convert(To_Type *null, const From_Type &from)
	{
		return convert(null, iconv_charset<From_Type>::rawptr(from),
		       iconv_charset<From_Type>::rawsize(from));
	}

	private:
	void doconvert(const char *lpFrom, size_t cbFrom, void *obj, void (*append)(void *, const char *, size_t));

	iconv_t	m_cd = reinterpret_cast<iconv_t>(-1);
	bool m_bForce = true; /* Ignore illegal sequences by default. */
	bool m_bHTML = false, m_translit_run = false;
	unsigned int m_translit_adv = 1;
};

/**
 * @brief	Allows multiple conversions within the same context.
 *
 * The convert_context class is used to perform multiple conversions within the
 * same context. This basically means that the iconv_context classes can
 * be reused, removing the need to recreate them for each conversion.
 */
class KC_EXPORT convert_context KC_FINAL {
public:
	convert_context() = default;
	/**
	 * @brief	Converts a string to a string with a different charset.
	 *
	 * convert_to comes in three forms:
	 *
	 * 1. convert_to<dsttype>(dstcharset, srcstring, srcsize, srccharset)
	 * 2. convert_to<dsttype>(srcstring, srcsize, srccharset)
	 *    autoderived: dstcharset
	 * 3. convert_to<dsttype>(srcstring)
	 *    autoderived: dstcharset, srcsize, srccharset
	 *
	 * Derivation happens with iconv_charset<> where the defaults are set.
	 *
	 * This is the function to call when a one of conversion from one charset to
	 * another is required.
	 * @tparam	  To_Type		The type of the destination string.
	 * @param[in] _from			The string that is to be converted to another charset.
	 * @return					The converted string.
	 *
	 * @note	Since this method needs to create an iconv object internally
	 *			it is better to use a convert_context when multiple conversions
	 *			need to be performed.
	 */
	template<typename To_Type, typename From_Type>
	To_Type convert_to(const From_Type &from)
	{
		static_assert(!std::is_same<To_Type, From_Type>::value, "pointless conversion");
		return get_context<To_Type, From_Type>(iconv_charset<To_Type>::name(), iconv_charset<From_Type>::name()).
		       convert(static_cast<To_Type *>(nullptr), from);
	}

	template<typename To_Type, typename From_Type>
	To_Type convert_to(const From_Type &from, size_t cbBytes, const char *fromcode)
	{
		return get_context<To_Type, From_Type>(iconv_charset<To_Type>::name(), fromcode).
		       convert(static_cast<To_Type *>(nullptr), iconv_charset<From_Type>::rawptr(from), cbBytes);
	}

	template<typename To_Type, typename From_Type>
	To_Type convert_to(const char *tocode, const From_Type &from,
	    size_t cbBytes, const char *fromcode)
	{
		return get_context<To_Type, From_Type>(tocode, fromcode).
		       convert(static_cast<To_Type *>(nullptr), iconv_charset<From_Type>::rawptr(from), cbBytes);
	}

private:
	/**
	 * @brief Key for the context_map;
	 */
	struct KC_EXPORT context_key {
		std::string totype, tocode, fromtype, fromcode;
		bool operator<(const context_key &o) const;
	};

	/** Create a context_key based on the to- and from types and optionally the to- and from codes.
	 *
	 * @tparam	To_Type
	 *			The destination type.
	 * @tparam	From_Type
	 *			The source type.
	 * @param[in]	tocode
	 *			The destination encoding. NULL for autodetect (based on To_Type).
	 * @param[in]	fromcode
	 *			The source encoding. NULL for autodetect (based onFrom_Type).
	 *
	 * @return	The new context_key
	 */
	template<typename To_Type, typename From_Type>
	KC_HIDDEN context_key create_key(const char *tocode,
	    const char *fromcode)
	{
		context_key key = {
			typeid(To_Type).name(),
			(tocode ? tocode : iconv_charset<To_Type>::name()),
			typeid(From_Type).name(),
			(fromcode ? fromcode : iconv_charset<From_Type>::name())
		};
		return key;
	}

	/**
	 * @brief Obtains an iconv_context object.
	 *
	 * The correct iconv_context is based on tocode and fromcode and is
	 * obtained from the context_map. If the correct iconv_context is not found, a new
	 * one is created and stored in the context_map;
	 * @param[in]	tocode		The destination charset.
	 * @param[in]	fromcode	The source charset.
	 * @return					A pointer to a iconv_context.
	 */
	template<typename To_Type, typename From_Type>
	KC_HIDDEN iconv_context &
	get_context(const char *tocode, const char *fromcode)
	{
		context_key key(create_key<To_Type, From_Type>(tocode, fromcode));
		auto iContext = m_contexts.find(key);
		if (iContext == m_contexts.cend())
			iContext = m_contexts.emplace(key, iconv_context(tocode, fromcode)).first;
		return iContext->second;
	}

	/**
	 * @brief Flags that determine which code of a context_key is persisted
	 */
	enum {
		pfToCode = 1,
		pfFromCode = 2
	};

	std::map<context_key, iconv_context> m_contexts;
	std::list<std::string>	m_lstStrings;
	std::list<std::wstring>	m_lstWstrings;

// a convert_context is not supposed to be copyable.
	convert_context(const convert_context &) = delete;
	convert_context &operator=(const convert_context &) = delete;
};

extern KC_EXPORT thread_local convert_context global_convert_context;

template<typename To_Type, typename From_Type>
inline To_Type convert_to(const From_Type &from)
{
	static_assert(!std::is_same<To_Type, From_Type>::value, "pointless conversion");
	return global_convert_context.convert_to<To_Type>(from);
}

template<typename To_Type, typename From_Type> inline To_Type
convert_to(const From_Type &from, size_t cbBytes, const char *fromcode)
{
	return global_convert_context.convert_to<To_Type>(iconv_charset<From_Type>::rawptr(from),
	       cbBytes, fromcode);
}

template<typename To_Type, typename From_Type>
inline To_Type convert_to(const char *tocode, const From_Type &from,
    size_t cbBytes, const char *fromcode)
{
	return global_convert_context.convert_to<To_Type>(tocode,
	       iconv_charset<From_Type>::rawptr(from), cbBytes, fromcode);
}

extern KC_EXPORT HRESULT HrFromException(const convert_exception &);

extern KC_EXPORT utf8string tfstring_to_utf8(const TCHAR *s, unsigned int flags);
extern KC_EXPORT std::string tfstring_to_lcl(const TCHAR *s, unsigned int flags);

#ifdef MAPIDEFS_H

/**
 * @brief	Converts a string from one charset to another. Failure is indicated
 *			through the return code instead of an exception.
 *
 * @param[in]  _from	The string to be converted.
 * @param[out] _to		The converted string.
 * @return				HRESULT.
 */
template<typename To_Type, typename From_Type>
HRESULT TryConvert(const From_Type &from, To_Type &to)
{
	try {
		to = convert_to<To_Type>(from);
		return hrSuccess;
	} catch (const convert_exception &ce) {
		return HrFromException(ce);
	}
}


/**
 * @brief	Converts a string from one charset to another. Failure is indicated
 *			through the return code instead of an exception.
 *
 * @param[in]  _from	The string to be converted.
 * @param[in] cbBytes	The size in bytes of the string to convert.
 * @param[in]  fromcode The source charset.
 * @param[out] _to		The converted string.
 * @return				HRESULT.
 */
template<typename To_Type, typename From_Type>
HRESULT TryConvert(const From_Type &from, size_t cbBytes,
    const char *fromcode, To_Type &to)
{
	try {
		to = convert_to<To_Type>(from, cbBytes, fromcode);
		return hrSuccess;
	} catch (const convert_exception &ce) {
		return HrFromException(ce);
	}
}

#endif // MAPIDEFS_H

} /* namespace */
