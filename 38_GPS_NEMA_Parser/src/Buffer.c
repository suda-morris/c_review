/*
 ============================================================================
 Name        : Buffer.c
 Author      : morris
 Version     :
 Copyright   : Your copyright notice
 Description :
 ============================================================================
 */
#include "Buffer.h"
#include <stdlib.h>
#include <string.h>

int8_t BUFFER_Init(BUFFER_t* Buffer, uint32_t Size, void* BufferPtr) {
	if (Buffer == NULL) {
		return -1;
	}
	memset(Buffer, 0, sizeof(BUFFER_t));

	Buffer->Size = Size;
	Buffer->Buffer = BufferPtr;
	Buffer->StringDelimiter = '\n'; //默认的字符串分隔符是换行

	if (!Buffer->Buffer) {
		Buffer->Buffer = (uint8_t *) LIB_ALLOC_FUNC(Size * sizeof(uint8_t));
		if (!Buffer->Buffer) {
			Buffer->Size = 0;
			return -2;
		} else {
			Buffer->Flags |= BUFFER_FLAG_MALLOC; /* Set flag for malloc */
		}
	}
	Buffer->Flags |= BUFFER_FLAG_INITIALIZED; /* We are initialized */

	return 0; /* Initialized OK */
}

void BUFFER_Free(BUFFER_t* Buffer) {
	if (Buffer == NULL) {
		return;
	}
	if (Buffer->Flags & BUFFER_FLAG_MALLOC) {
		LIB_FREE_FUNC(Buffer->Buffer);
	}
	Buffer->Flags = 0;
	Buffer->Size = 0;
}

void BUFFER_Reset(BUFFER_t* Buffer) {
	if (Buffer == NULL) {
		return;
	}
	Buffer->In = 0; /* Reset values */
	Buffer->Out = 0;
}

uint32_t BUFFER_GetFree(BUFFER_t* Buffer) {
	uint32_t size = 0, in, out;

	if (Buffer == NULL) {
		return 0;
	}
	in = Buffer->In; /* Save values */
	out = Buffer->Out;

	size = ((Buffer->Size) + out - in - 1) % Buffer->Size;
	return size; /* Return free memory */
}

uint32_t BUFFER_GetFull(BUFFER_t* Buffer) {
	uint32_t size = 0, in, out;

	if (Buffer == NULL) { /* Check buffer structure */
		return 0;
	}
	in = Buffer->In; /* Save values */
	out = Buffer->Out;
	size = (Buffer->Size + in - out) % Buffer->Size;
	return size;
}

uint32_t BUFFER_Write(BUFFER_t* Buffer, const void* Data, uint32_t count) {
	uint32_t i = 0;
	uint32_t free;
	const uint8_t* d = (const uint8_t *) Data;
	uint32_t tocopy;

	if (Buffer == NULL || count == 0) {
		return 0;
	}
	if (Buffer->In >= Buffer->Size) { /* Check input pointer */
		Buffer->In = 0;
	}
	free = BUFFER_GetFree(Buffer); /* Get free memory */
	if (free < count) { /* Check available memory */
		if (free == 0) { /* If no memory, stop execution */
			return 0;
		}
		count = free; /* Set values for write */
	}

	tocopy = Buffer->Size - Buffer->In; /* Calculate number of elements we can put at the end of buffer */
	if (tocopy > count) { /* Check for copy count */
		tocopy = count;
	}
	memcpy(&Buffer->Buffer[Buffer->In], d, tocopy);
	i += tocopy; /* Increase number of bytes we copied already */
	Buffer->In += tocopy;
	count -= tocopy;
	if (count > 0) { /* Check if anything to write */
		memcpy(Buffer->Buffer, (void *) &d[i], count); /* Copy content */
		Buffer->In = count; /* Set input pointer */
	}
	if (Buffer->In >= Buffer->Size) { /* Check input overflow */
		Buffer->In = 0;
	}
	return (i + count); /* Return number of elements stored in memory */
}

uint32_t BUFFER_WriteString(BUFFER_t* Buffer, const char* buff) {
	return BUFFER_Write(Buffer, (uint8_t *) buff, strlen(buff));
}

uint32_t BUFFER_WriteToTop(BUFFER_t* Buffer, const void* Data, uint32_t count) {
	uint32_t i = 0;
	uint32_t free;
	uint8_t *d = (uint8_t *) Data;

	if (Buffer == NULL || count == 0) {
		return 0;
	}
	if (Buffer->In >= Buffer->Size) { /* Check input pointer */
		Buffer->In = 0;
	}
	if (Buffer->Out >= Buffer->Size) { /* Check output pointer */
		Buffer->Out = 0;
	}
	free = BUFFER_GetFree(Buffer); /* Get free memory */
	if (free < count) { /* Check available memory */
		if (free == 0) { /* If no memory, stop execution */
			return 0;
		}
		count = free; /* Set values for write */
	}
	d += count - 1; /* Start on bottom */
	while (count--) { /* Go through all elements */
		if (Buffer->Out == 0) { /* Check output pointer */
			Buffer->Out = Buffer->Size - 1;
		} else {
			Buffer->Out--;
		}
		Buffer->Buffer[Buffer->Out] = *d--; /* Add to buffer */
		i++; /* Increase pointers */
	}
	return i; /* Return number of elements written */
}

uint32_t BUFFER_Read(BUFFER_t* Buffer, void* Data, uint32_t count) {
	uint32_t i = 0, full;
	uint8_t *d = (uint8_t *) Data;
	uint32_t tocopy;

	if (Buffer == NULL || count == 0) { /* Check buffer structure */
		return 0;
	}
	if (Buffer->Out >= Buffer->Size) { /* Check output pointer */
		Buffer->Out = 0;
	}
	full = BUFFER_GetFull(Buffer); /* Get free memory */
	if (full < count) { /* Check available memory */
		if (full == 0) { /* If no memory, stop execution */
			return 0;
		}
		count = full; /* Set values for write */
	}

	tocopy = Buffer->Size - Buffer->Out; /* Calculate number of elements we can read from end of buffer */
	if (tocopy > count) { /* Check for copy count */
		tocopy = count;
	}
	memcpy(d, &Buffer->Buffer[Buffer->Out], tocopy); /* Copy content from buffer */
	i += tocopy; /* Increase number of bytes we copied already */
	Buffer->Out += tocopy;
	count -= tocopy;
	if (count > 0) { /* Check if anything to read */
		memcpy(&d[i], Buffer->Buffer, count); /* Copy content */
		Buffer->Out = count; /* Set input pointer */
	}
	if (Buffer->Out >= Buffer->Size) { /* Check output overflow */
		Buffer->Out = 0;
	}
	return (i + count); /* Return number of elements stored in memory */
}

int32_t BUFFER_FindElement(BUFFER_t* Buffer, uint8_t Element) {
	uint32_t Num, Out, retval = 0;

	if (Buffer == NULL) {
		return -1;
	}

	Num = BUFFER_GetFull(Buffer); /* Create temporary variables */
	Out = Buffer->Out;
	while (Num > 0) { /* Go through input elements */
		if (Out >= Buffer->Size) { /* Check output overflow */
			Out = 0;
		}
		if ((uint8_t) Buffer->Buffer[Out] == (uint8_t) Element) { /* Check for element */
			return retval; /* Element found, return position in buffer */
		}
		Out++; /* Set new variables */
		Num--;
		retval++;
	}
	return -1; /* Element is not in buffer */
}

int8_t BUFFER_CheckElement(BUFFER_t* Buffer, uint32_t pos, uint8_t* element) {
	uint32_t In, Out, i = 0;
	if (Buffer == NULL) {
		return 0;
	}

	In = Buffer->In;
	Out = Buffer->Out;
	while (i < pos && (In != Out)) { /* Set pointers to right location */
		Out++; /* Increase output pointer */
		i++;
		if (Out >= Buffer->Size) { /* Check overflow */
			Out = 0;
		}
	}
	if (i == pos) { /* If positions match */
		*element = Buffer->Buffer[Out]; /* Save element */
		return 0;
	}
	return -1;
}

uint32_t BUFFER_ReadString(BUFFER_t* Buffer, char* buff, uint32_t buffsize) {
	uint32_t i = 0, freeMem, fullMem;
	uint8_t ch;
	if (Buffer == NULL) {
		return 0;
	}

	freeMem = BUFFER_GetFree(Buffer); /* Get free memory */
	fullMem = BUFFER_GetFull(Buffer); /* Get full memory */
	if (fullMem == 0
			|| (BUFFER_FindElement(Buffer, Buffer->StringDelimiter) < 0 && /* String delimiter is not in buffer */
			freeMem != 0 && fullMem < buffsize /* User buffer size is larger than number of elements in buffer */
			)) {
		return 0; /* Return with no elements read */
	}
	while (i < (buffsize - 1)) { /* If available buffer size is more than 0 characters */
		BUFFER_Read(Buffer, &ch, 1); /* We have available data */
		buff[i] = (char) ch; /* Save character */
		if ((char) buff[i] == (char) Buffer->StringDelimiter) { /* Check for end of string */
			break; /* Done */
		}
		i++; /* Increase */
	}
	if (i == (buffsize - 1)) { /* Add zero to the end of string */
		buff[i] = 0;
	} else {
		buff[++i] = 0;
	}
	return i; /* Return number of characters in buffer */
}

int32_t BUFFER_Find(BUFFER_t* Buffer, const void* Data, uint32_t Size) {
	uint32_t Num, Out, i, retval = 0;
	uint32_t temp_Out, temp_Num;
	uint8_t found = 0;
	uint8_t* d = (uint8_t *) Data;

	if (Buffer == NULL || (Num = BUFFER_GetFull(Buffer)) < Size) {
		return -1;
	}
	Out = Buffer->Out; /* Create temporary variables */
	while (Num > 0) { /* Go through input elements in buffer */
		if (Out >= Buffer->Size) { /* Check output overflow */
			Out = 0;
		}
		if ((uint8_t) Buffer->Buffer[Out] == d[0]) {
			found = 1;
		}

		Out++; /* Set new variables */
		Num--;
		retval++;
		if (found) { /* We have found first element */
			temp_Out = Out;
			temp_Num = Num;
			i = 1; /* First character found */
			while (i < Size && Num > 0) { /* Check others */
				if (Out >= Buffer->Size) { /* Check output overflow */
					Out = 0;
				}
				if ((uint8_t) Buffer->Buffer[Out] != d[i]) {
					Num = temp_Num;
					Out = temp_Out;
					break;
				}
				Out++; /* Set new variables */
				Num--;
				i++;
			}
			if (i == Size) { /* We have found data sequence in buffer */
				return retval - 1;
			} else if (Num <= 0) {
				return -1;
			}
			found = 0;
		}
	}
	return -1; /* Data sequence is not in buffer */
}

