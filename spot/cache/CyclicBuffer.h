#ifndef SPOT_CACHE_CYCLICBUFFER_H
#define SPOT_CACHE_CYCLICBUFFER_H
#include <spot/utility/Compatible.h>
#include<spot/cache/CacheNodeList.h>
#include<vector>

//֧��һдһ�����̲߳��������뱣֤��д˳��һ�£����Խ��spsc_queueʹ��
class CyclicBuffer
{
public:
	static const uint32_t kInitialSize = 16 * 1024 * 1024;
	CyclicBuffer(std::string bufferName, uint32_t nodeSize = kInitialSize);
	~CyclicBuffer();
	char* allocate(uint32_t length);
	void  release(uint32_t length);
  private:
	  CacheNodeList *nodeList_;	 
};
#endif  

