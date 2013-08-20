/*
 * Copyright (C) 2013 NEC Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2, as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */


#ifndef COMMON_DEFS_H
#define COMMON_DEFS_H


#ifdef __cplusplus
extern "C" {
#define CLOSE_EXTERN }
#else
#define CLOSE_EXTERN
#endif


enum service_idx {
  internet_service_idx,
  video_service_idx,
  phone_service_idx
};

#define SERVICES_MAX ( phone_service_idx + 1 )

static const char * const service_names[] = {
  "Internet_Access_Service",
  "Video_Service",
  "Phone_Service"
};

#define SERVICE_NAMES_MAX ( phone_service_idx + 1 )
#define INTERNET_ACCESS_SERVICE service_names[ internet_service_idx ]
#define VIDEO_SERVICE  service_names[ video_service_idx ]
#define PHONE_SERVICE service_names[ phone_service_idx ]

static const char * const service_modules[] = {
  "internet_service_module",
  "video_service_module",
  "phone_service_module"
};

#define SERVICE_MODULES_MAX ( phone_service_idx + 1 )
#define INTERNET_SERVICE_MODULE service_modules[ internet_service_idx ]
#define VIDEO_SERVICE_MODULE service_modules[ video_service_idx ]
#define PHONE_SERVICE_MODULE service_modules[ phone_service_idx ]

static const char * const chef_recipes[] = {
  "internet_recipe",
  "video_recipe",
  "phone_recipe"
};

#define CHEF_RECIPES_MAX ( phone_service_idx + 1 )
#define INTERNET_CHEF_RECIPE chef_recipes[ internet_service_idx ]
#define VIDEO_CHEF_RECIPE chef_recipes[ video_service_idx ]
#define PHONE_CHEF_RECIPE chef_recipes[ phone_service_idx ]


CLOSE_EXTERN
#endif // COMMON_DEFS_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
