### 학습내용
- uniform buffer
- 디스크립터
- 디스크립터 셋
- 디스크립터 풀
- 디스크립터 셋 레이아웃
- 파이프라인 레이아웃
<br>

---
### DirectX12 대응
- 디스크립터 				<-> 디스크립터(뷰)
- 디스크립터 풀			<-> 디스크립터 힙
- 디스크립터 셋			<-> X
- 디스크립터 셋 레이아웃	<-> 루트파라미터, 디스크립터 테이블(루트 테이블)
- ?다이나믹 오프셋?		<-> 루트파라미터, 인라인 디스크립터(루트 디스크립터)
- ?푸시 상수?			<-> 루트파라미터, 인라인 상수(루트 상수)
- 파이프라인 레이아웃		<-> 루트시그니쳐

---
### Motivation: OpenGL에서 유니폼 버퍼 바인딩
```cpp
// 유니폼 버퍼 생성
GLuint ubo;
glGenBuffers(1, &ubo);
glBindBuffer(GL_UNIFORM_BUFFER, ubo);
glBufferData(GL_UNIFORM_BUFFER, bufferSize, nullptr, GL_STATIC_DRAW);

// 데이터 매핑 및 업로드
void* mappedData = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
memcpy(mappedData, &data, sizeof(data)); 
glUnmapBuffer(GL_UNIFORM_BUFFER);    


// 유니폼 버퍼를 Binding Point에 연결
glBindBufferBase(GL_UNIFORM_BUFFER, 0, ubo);

// 렌더링
glUseProgram(shaderProgram);
glDrawArrays(GL_TRIANGLES, 0, vertexCount);
```

---
### Vulkan에서 유니폼 버퍼 바인딩 핵심 요약
쉐이더 <-> 파이프라인(디스크립터 셋) 레이아웃 <-> 디스크립터 셋 <-> 버퍼

---
### Vulkan에서 유니폼 버퍼 바인딩 전체 과정 
1. 유니폼 버퍼 생성 및 데이터 업로드
	- 유니폼 데이터를 저장할 VkBuffer를 생성하고, GPU 메모리를 할당 및 바인딩.
	- vkMapMemory로 버퍼 메모리를 매핑하여 데이터를 복사하고, vkUnmapMemory로 매핑을 해제.

2. Descriptor Set Layout 정의
	- 유니폼 버퍼를 쉐이더에 연결하기 위해 VkDescriptorSetLayout을 생성.
	- 각 유니폼의 바인딩 슬롯, 타입, 사용할 쉐이더 스테이지 등을 정의.
	
3. Descriptor Pool 생성
	- Descriptor Set을 관리할 VkDescriptorPool을 생성.
	- 필요한 Descriptor 유형과 최대 개수, 최대 세트 수를 설정.

4. Descriptor Set 할당
	- Descriptor Pool에서 VkDescriptorSet을 할당.
	- 각 Descriptor Set은 해당 쉐이더 슬롯에 매핑될 리소스를 참조.

5. Descriptor Set에 유니폼 버퍼 바인딩
	- 유니폼 버퍼 정보를 담은 VkDescriptorBufferInfo를 생성.
	- vkUpdateDescriptorSets를 호출해 Descriptor Set과 유니폼 버퍼를 연결.

6. Command Buffer에서 Descriptor Set 바인딩
	- vkCmdBindDescriptorSets를 사용해 Descriptor Set을 커맨드 버퍼에 바인딩.

7. 렌더링
