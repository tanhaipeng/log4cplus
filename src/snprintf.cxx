#include <log4cplus/helpers/snprintf.h>
#include <log4cplus/helpers/loglog.h>
#include <log4cplus/internal/internal.h>
#include <cstdarg>
#include <cstdio>
#if defined (UNICODE)
#include <cwchar>
#endif
#if defined (LOG4CPLUS_HAVE_STDARG_H)
#  include <stdarg.h>
#endif
#if defined (LOG4CPLUS_HAVE_WCHAR_H)
#  include <wchar.h>
#endif
#if defined (LOG4CPLUS_HAVE_STDIO_H)
#  include <stdio.h>
#endif


namespace log4cplus { namespace helpers {


size_t const START_BUF_SIZE = 512;

#if defined (WIN32)
char const NULL_FILE[] = "NUL";
#else
char const NULL_FILE[] = "/dev/null";
#endif


namespace
{


static inline
int
vftprintf (std::FILE * file, tchar const * fmt, std::va_list args)
{
#if defined (UNICODE)
#  if defined (LOG4CPLUS_HAVE_VFWPRINTF_S)
    return vfwprintf_s (file, fmt, args);
#  else
    return std::vfwprintf (file, fmt, args);
#  endif
#else
#  if defined (LOG4CPLUS_HAVE_VFPRINTF_S)
    return vfprintf_s (file, fmt, args);
#  else
    return std::vfprintf (file, fmt, args);
#  endif
#endif
}


static inline
int
vstprintf (tchar * dest, std::size_t dest_size, tchar const * fmt,
    std::va_list args)
{
    int ret;
#if defined (UNICODE)
#  if defined (LOG4CPLUS_HAVE_VSWPRINTF_S)
    ret = vswprintf_s (dest, dest_size, fmt, args);
#  else
    ret = std::vswprintf (dest, dest_size, fmt, args);
#  endif
#else
#  if defined (LOG4CPLUS_HAVE_VSPRINTF_S)
    ret = vsprintf_s (dest, dest_size, fmt, args);
#  else
    ret = std::vsprintf (dest, fmt, args);
#  endif
#endif

    if (ret >= 0)
        assert (static_cast<std::size_t>(ret) <= dest_size);
    
    return ret;
}


#if ! defined (LOG4CPLUS_USE_POOR_MANS_SNPRINTF)
static inline
int
vsntprintf (tchar * dest, std::size_t dest_size, tchar const * fmt,
    std::va_list args)
{
    int ret;

#if defined (UNICODE)
#  if defined (LOG4CPLUS_HAVE__VSNWPRINTF_S) && defined (_TRUNCATE)
    ret = _vsnwprintf_s (dest, dest_size, _TRUNCATE, fmt, args);
#  else
    ret = std::vswprintf (dest, dest_size, fmt, args);
#  endif
#else
#  if defined (LOG4CPLUS_HAVE_VSNPRINTF_S) && defined (_TRUNCATE)
    ret = vsnprintf_s (dest, dest_size, _TRUNCATE, fmt, args);
#  elif defined (LOG4CPLUS_HAVE__VSNPRINTF_S) && defined (_TRUNCATE)
    ret = _vsnprintf_s (dest, dest_size, _TRUNCATE, fmt, args);
#  elif defined (LOG4CPLUS_HAVE_VSNPRINTF)
    ret = vsnprintf (dest, dest_size, fmt, args);
#  elif defined (LOG4CPLUS_HAVE__VSNPRINTF)
    ret = _vsnprintf (dest, dest_size, fmt, args);
#  else
#    error "None of vsnprintf_s, _vsnprintf_s, vsnprintf or _vsnprintf is available."
#  endif
#endif
    
    return ret;
}
#endif


}


snprintf_buf::snprintf_buf ()
    : buf (START_BUF_SIZE)
{ }


tchar const *
snprintf_buf::print (tchar const * fmt, ...)
{
    assert (fmt);

    std::va_list args;
    va_start (args, fmt);
    tchar const * ret = print (fmt, args);
    va_end (args);
    return ret;
}


tchar const *
snprintf_buf::print (tchar const * fmt, std::va_list args)
{
    int printed;
    size_t const fmt_len = std::char_traits<tchar>::length (fmt);
    size_t buf_size = buf.size ();
    size_t const output_estimate = fmt_len + fmt_len / 2 + 1;
    if (output_estimate > buf_size)
        buf.resize (output_estimate);

#if defined (LOG4CPLUS_USE_POOR_MANS_SNPRINTF)
    std::FILE * & fnull = internal::get_ptd ()->fnull;
    if (! fnull)
    {
        fnull = std::fopen (NULL_FILE, "wb");
        if (! fnull)
        {
            LogLog::getLogLog ()->error (
                LOG4CPLUS_TEXT ("Could not open NULL_FILE."));
            buf.clear ();
            buf.push_back (0);
            return &buf[0];
        }
    }
   
    printed = vftprintf (fnull, fmt, args);
    if (printed == -1)
    {
        LogLog::getLogLog ()->error (
            LOG4CPLUS_TEXT ("Error printing into NULL_FILE."));
        buf.clear ();
        buf.push_back (0);
        return &buf[0];
    }

    buf.resize (printed + 1);
    int sprinted = vstprintf (&buf[0], buf.size (), fmt, args);
    if (sprinted == -1)
    {
        LogLog::getLogLog ()->error (
            LOG4CPLUS_TEXT ("Error printing into string."));
        buf.clear ();
        buf.push_back (0);
        return &buf[0];
    }
    assert (printed == sprinted);
    
    buf[sprinted] = 0;

#else
    do
    {
        printed = vsntprintf (&buf[0], buf_size - 1, fmt, args);
        if (printed == -1)
        {
            buf_size *= 2;
            buf.resize (buf_size);
        }
        else
            buf[printed] = 0;
    }
    while (printed == -1);

#endif

    return &buf[0];
}


} } // namespace log4cplus { namespace helpers
