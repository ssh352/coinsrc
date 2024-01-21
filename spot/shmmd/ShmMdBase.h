#ifndef SPOT_SHMMD_SHMMDBASE_H
#define SPOT_SHMMD_SHMMDBASE_H

#include <spot/base/MqStruct.h>

/*         �����ڴ�ṹ˵��
+ ---------------- + ------------ +
|  ShmBufferSize   |  �����ڴ��С |
+ ---------------- + ------------ +
| InstrumentCount  |   ��Լ����	  |
+ ---------------- + ------------ +
|    ShmMdIndex    |   ��������    |
+ ---------------- + ------------ +
|     ʡ��(InstrumentCount-1��)    |
+ ---------------- + ------------ +
|  InnerMarketData |   ��������    |
+ ---------------- + ------------ +
|     ʡ��(InstrumentCount-1��)    |
+ ---------------- + ------------ +
*/

namespace spot
{
	namespace shmmd
	{
		struct ShmMdIndex
		{
			char InstrumentID[spot::base::InstrumentIDLen + 1];
			int Offset; //�����ڴ��׵�ַ �� InnerMarketData ��ƫ��
		};

	}
}

#endif //SPOT_SHMMD_SHMMDBASE_H
