#ifndef __COMPAT_VERSION_H__
#   define __COMPAT_VERSION_H__

#if defined(RHEL_RELEASE_CODE) && defined(RHEL_RELEASE_VERSION)
#   if RHEL_RELEASE_CODE >= RHEL_RELEASE_VERSION(10, 1)
#      define RHEL_10_1_BACKPORTS 1
#   endif
#endif

#endif /* __COMPAT_VERSION_H__ */