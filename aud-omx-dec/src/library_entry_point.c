#include <bellagio/st_static_component_loader.h>
#include <config.h>
#include "omx_audiodec_component.h"

/*
  Copyright (C) 2007-2009 STMicroelectronics
  Copyright (C) 2007-2009 Nokia Corporation and/or its subsidiary(-ies)

  This library is free software; you can redistribute it and/or modify it under
  the terms of the GNU Lesser General Public License as published by the Free
  Software Foundation; either version 2.1 of the License, or (at your option)
  any later version.

  This library is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
  FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
  details.

  You should have received a copy of the GNU Lesser General Public License
  along with this library; if not, write to the Free Software Foundation, Inc.,
  51 Franklin St, Fifth Floor, Boston, MA
  02110-1301  USA

  $Date: 2009-10-09 14:34:15 +0200 (Fri, 09 Oct 2009) $
  Revision $Rev: 875 $
  Author $Author: gsent $

  You can also contact with mail(gangx.li@intel.com)
*/

// The library entry point. It must have the same name for each
// library of the components loaded by the ST static component loader.
int omx_component_library_Setup(stLoaderComponentType **stComponents) {

	OMX_U32 i;
	DEBUG(DEB_LEV_FUNCTION_NAME, "In %s \n",__func__);

	if (stComponents == NULL) {
		DEBUG(DEB_LEV_FUNCTION_NAME, "Out of %s \n",__func__);
		return 1;
	}

	// component audio decoder
	stComponents[0]->componentVersion.s.nVersionMajor = 1;
	stComponents[0]->componentVersion.s.nVersionMinor = 1;
	stComponents[0]->componentVersion.s.nRevision = 1;
	stComponents[0]->componentVersion.s.nStep = 1;

	stComponents[0]->name = calloc(1, OMX_MAX_STRINGNAME_SIZE);
	if (stComponents[0]->name == NULL) {
		return OMX_ErrorInsufficientResources;
	}

	strcpy(stComponents[0]->name, "OMX.st.audio_decoder");
	stComponents[0]->name_specific_length = 2;
	stComponents[0]->constructor = omx_audiodec_component_Constructor;

	stComponents[0]->name_specific = calloc(stComponents[0]->name_specific_length,sizeof(char *));
	stComponents[0]->role_specific = calloc(stComponents[0]->name_specific_length,sizeof(char *));

	for(i=0;i<stComponents[0]->name_specific_length;i++) {
		stComponents[0]->name_specific[i] = calloc(1, OMX_MAX_STRINGNAME_SIZE);
		if (stComponents[0]->name_specific[i] == NULL) {
			return OMX_ErrorInsufficientResources;
		}
	}
	for(i=0;i<stComponents[0]->name_specific_length;i++) {
		stComponents[0]->role_specific[i] = calloc(1, OMX_MAX_STRINGNAME_SIZE);
		if (stComponents[0]->role_specific[i] == NULL) {
			return OMX_ErrorInsufficientResources;
		}
	}

	strcpy(stComponents[0]->name_specific[1], "OMX.st.audio_decoder.ac3");
	strcpy(stComponents[0]->role_specific[1], "audio_decoder.ac3");

	DEBUG(DEB_LEV_FUNCTION_NAME, "Out of %s \n",__func__);

	return 1;

}
