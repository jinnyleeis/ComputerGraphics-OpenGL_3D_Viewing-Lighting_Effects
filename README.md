
## Table of Contents
* [Keyboard Controls](#keyboard-controls)
* [OpenGL API 3D Viewing](#hw3--opengl-api-3d-뷰잉-연습)
  * [요구사항 구현 요약](#hw3-요구사항-구현-요약)
  * [추가 구현](#hw3-추가-구현)
  * [요구사항별 설명](#hw3-요구사항별-설명)
    * [(a)i) 정적 기하 물체 5개 배치](#hw3-2a-i)
    * [(a)ii) 동적 기하 물체 2개 이동 구현](#hw3-2a-ii)
    * [(b)i) 주 카메라 축 이동](#hw3-2b-i)
    * [(b)ii) 주 카메라 축 회전](#hw3-2b-ii)
    * [(b)iii) 줌 인·아웃](#hw3-2b-iii)
    * [(b)iv) 정적 CCTV 카메라 3개](#hw3-2b-iv)
    * [(b)v) 동적 CCTV 카메라 1개](#hw3-2b-v)
    * [(b)vi) 직교투영 3개](#hw3-2b-vi)
    * [(b)vii) 카메라 프레임 토글](#hw3-2b-vii)
    
* [Lighting & Effects](#hw5--lighting--effects)

  * [요구사항 구현 요약](#hw5-요구사항-구현-요약)
  * [상세 구현 설명](#hw5-상세-구현-설명)
    * [1. Gouraud / Phong Shading](#hw5-1)
    * [2. 영상 텍스처 매핑 & 필터 토글](#hw5-2)
    * [3. 월드 좌표계 광원](#hw5-3)
    * [4. 눈 좌표계 광원](#hw5-4)
    * [5. 모델 좌표계 광원](#hw5-5)
    * [6. 투명 다면체](#hw5-6)
    * [7. 창의적 쉐이더 효과 2종](#hw5-7)

---


## Keyboard Controls<a name="keyboard-controls"></a>

다음 키보드 입력으로 각 기능을 제어할 수 있습니다:

| 키                     | 설명                            |
| --------------------- | ----------------------------- |
| 1 / 2                 | CAT Gouraud / Phong Shading   |
| N / L                 | 텍스처 필터 Nearest / Linear       |
| 3                     | 월드 광원 ON/OFF                  |
| 4                     | 카메라 고정 광원 ON/OFF              |
| 5                     | Tiger 헤드라이트(모델링 좌표계 광원) ON/OFF            |
| 6, +, −               | 20‑면체 블렌딩 토글 & 알파 조절          |
| 7 / 8                 | Dragon Lava / Ironman Fresnel |
| ← / → / ↑ / ↓         | Wolf 이동 (Main Camera 추적 대상 이동)                      |
| A / D                 | Main Camera 좌/우 둘러보기 (leftRight)        |
| W / S                 | Main Camera 위/아래 보기 (upDown)       |
| Q / E                 | Main Camera 머리 기울이기 (headTilt)      |
| Z / X                 | Main/CCTV-D 확대(Z) 또는 축소(X) (Zoom In/Out)               |
| V / B                 | CCTV-D 카메라 FOV 세밀 조정                |
| 0                     | Main ↔ CCTV‑D 전환              |
| Y                     | 카메라 축(Frame) 표시 토글                 |
| Esc                   | 프로그램 종료                       |

---

## 1.OpenGL API 3D 뷰잉<a name="hw3--opengl-api-3d-뷰잉-연습"></a>

### 요구사항 구현 요약<a name="hw3-요구사항-구현-요약"></a>

| 요구사항 항목                                    | 구현 여부             | 
| ------------------------------------------ | ----------------- | 
| (a)i) 정적 기하 물체 5개 이상 배치                   | ✔️                | 
| (a)ii) 동적 기하 물체 2개 이동 구현 (Tiger & Spider) | ✔️                | 
| (b)i) 주 카메라 축 이동                          | ✔️ Wolf 추적        |
| (b)ii) 주 카메라 축 회전 (Yaw/Pitch/Roll)        | ✔️                | 
| (b)iii) 줌 인·아웃 (Z/X)                      | ✔️                | 
| (b)iv) 정적 CCTV 카메라 3개 (A,B,C)             | ✔️                | 
| (b)v) 동적 CCTV 카메라 1개 (D)                  | ✔️ 뷰포트 & 제어       | 
| (b)vi) 직교투영 3개 (정·측·상)                    | ✔️ Front/Side/Top | 
| (b)vii) 카메라 프레임 토글 ('Y')                  | ✔️                | 

### 추가 구현<a name="hw3-추가-구현"></a>
'카메라 충돌 회피 기능 구현 + 늑대 시야를 주 카메라로 설정'

```
늑대의 약간 윗부분에 주 카메라를 계층적 모델링을 통해 배치하였고,
floor_mask를 참조하여 이동 가능한지 검사하고, 벽(cell==1)일 경우 이동을 무시할 수 있도록 하여,
주 카메라가 벽이나 물체를 뚫고 가지 않도록 구현했습니다.
주 카메라는, 건물 밖을 뚫고 나갈 수 없으며, 건물 내부에서 움직입니다.
```
<img width="1283" alt="image" src="https://github.com/user-attachments/assets/970166ae-5cd2-4a46-be18-2e6e0db004c8" />
<img width="1098" alt="image" src="https://github.com/user-attachments/assets/b5dc4add-aa65-4029-8e8e-5f898a5442ce" />



### 요구사항별 설명<a name="hw3-요구사항별-설명"></a>

#### (a)i) 정적 기하 물체 5개 배치<a name="hw3-2a-i"></a>
<img width="964" alt="image" src="https://github.com/user-attachments/assets/834a9a7d-9cf5-4c38-a26c-ae00c6867158" />


#### (a)ii) 동적 기하 물체 2개 이동 구현<a name="hw3-2a-ii"></a>
<img width="832" alt="image" src="https://github.com/user-attachments/assets/5ad226dc-df74-4c32-a199-34de8a03b8ae" />


```
Tiger와 Spider 객체가 지정된 경로(PathSeg)를 따라 자동으로 이동합니다.
해당 경로는 벽을 뚫지 않는, 자연스러운 이동 경로로 설정하였습니다.
tiger는 바닥을 걷는 호랑의 자연스러운 움직임을, 거미는 바닥을 굴러다니는 움직임을 통해 동적 애니메이션을 생성하였습니다. 
draw_object() 내부의 path_pos_dir() 호출로 경로를 따라 위치와 방향을 계산하고, ModelMatrix를 갱신합니다.

실행 후 타이머에 의해 특정 주기마다 draw()가 호출되어,
Tiger와 Spider가 미리 정의된 PathSeg 배열을 따라 자동으로 순환하도록 구현했습니다.
```

#### (b)i) 주 카메라 축 이동<a name="hw3-2b-i"></a>

```
설명: Main Camera는 상하좌우 키를 통해 이동이 가능합니다.
```

#### (b)ii) 주 카메라 축 회전<a name="hw3-2b-ii"></a>
<img width="736" alt="image" src="https://github.com/user-attachments/assets/8afc635a-2bf4-470f-b2d7-b94b61ec1c0d" />

```
Main Camera의 로컬 축(u, v, n) 둘레 회전 기능을 구현했습니다.
A / D : Main Camera 좌/우 둘러보기 (leftRight)
W / S : Main Camera 위/아래 보기 (upDown)
Q / E : Main Camera 머리 기울이기 (headTilt)
주 카메라를 늑대의 시야라고 가정하였으므로, 360도 회전이 되는 움직임은 자연스럽지 않아, 각각의 회전의 범위를 지정해주었습니다. 
특히, 위 아래로 둘러보기 기능에서, 아래로 기울어지는 것을 큰 범위를 허용해주면,
카메라가 늑대 오브젝트를 뚫고 들어가 늑대 오브젝트 와이프레임 내부를 보여주게 되므로,
이땐, 더 범위를 좁게 *0.15f를 곱하여 지정했습니다.
```

#### (b)iii) 줌 인·아웃<a name="hw3-2b-iii"></a>

```
원근 투영 시 Field of View(FOV)를 조절하여 확대/축소 효과를 구현했습니다.
작동법: Z 키로 줌 인, X 키로 줌 아웃을 수행할 수 있습니다.
```

#### (b)iv) 정적 CCTV 카메라 3개<a name="hw3-2b-iv"></a>

```
CCTV-A/B/C 카메라는 건물 위쪽에 고정된 위치에서 건물 내부를 관찰합니다.
프로그램 실행 시 좌측 상단 3개의 작은 뷰포트에 각각 A, B, C 뷰가 표시됩니다.
```

#### (b)v) 동적 CCTV 카메라 1개<a name="hw3-2b-v"></a>
<img width="716" alt="image" src="https://github.com/user-attachments/assets/7ca72bb3-a26d-4bfa-b7fa-189c1395146f" />

```
CCTV-D 카메라는 고정된 위치에서 키 입력으로 회전 및 FOV 조절이 가능합니다.
작동법: 0 키로 Main ⇄ CCTV-D 전환, W/S/A/D/Q/E로 회전(main과 동일), V/B로 FOV 조절(동적 카메라에 추가).
z 키로 줌 인, x 키로 줌 아웃은 메인과 동일하게 작동 가능합니다. 
```

#### (b)vi) 직교투영 3개<a name="hw3-2b-vi"></a>

```
Front, Side, Top 카메라가 각각 정면도, 측면도, 상면도를 표시합니다.
작동법: 실행 시 우측 상단에 3개의 뷰포트에서 직교 투영 화면이 함께 표시됩니다. 
```

#### (b)vii) 카메라 프레임 토글<a name="hw3-2b-vii"></a>
<img width="920" alt="image" src="https://github.com/user-attachments/assets/9ea63308-9f70-4abc-aff3-0f344a3f6550" />

```
직교 투영 카메라를 제외한 모든 카메라 프레임을 토글할 수 있습니다.
작동법: Y 키를 눌러 축 표시(on/off)를 전환할 수 있습니다。
```


---

## Lighting & Effects<a name="hw5--lighting--effects"></a>

### 요구사항 구현 요약<a name="hw5-요구사항-구현-요약"></a>

| 항목 (번호)                                    | 구현 여부 | 비고                            |
| ------------------------------------------ | ----- | ----------------------------- |
| 1. Gouraud / Phong Shading (키 1,2)         | ✔️    | 고양이(CAT) 모델                   |
| 2. 영상 텍스처 매핑 (정적 1 + 동적 1) & 필터 토글 (키 N/L) | ✔️     | Wood Tower & Spider           |
| 3. 월드 좌표계 광원 (키 3)                         | ✔️      | 천장 램프 3개                      |
| 4. 눈 좌표계(카메라 고정) 광원 (키 4)                  | ✔️       | CCTV‑D 플래시                    |
| 5. 모델 좌표계 광원 (키 5)                         | ✔️       | Tiger 헤드라이트                   |
| 6. 투명 다면체 + 알파 조절 (키 6 ±)                  | ✔️        | 정 20‑면체                       |
| 7. 창의적 쉐이더 효과 2종 (키 7,8)                   | ✔️        | Dragon Lava / Ironman Fresnel |

### 상세 구현 설명<a name="hw5-상세-구현-설명"></a>

#### 1. Gouraud / Phong Shading<a name="hw5-1"></a>
<img width="1023" alt="image" src="https://github.com/user-attachments/assets/b3a9d863-174e-4527-9aac-de3081d18699" />

```
임의 물체: 고양이(CAT). gouraud.vert / phong.vert 셰이더를 준비하고
'1'(Gouraud) ↔ '2'(Phong) 키로 g_shading_mode를 변경,
Static_Object::draw_object()에서 각 셰이더를 선택적으로 바인딩합니다.
고라드에선, 버텍스에서 라이팅을, 일반 라이팅 퐁에선, fragd에서 라이팅을 계산하여, 세밀도의 차이가 확연히 드러납니다. 
```

#### 2. 영상 텍스처 매핑 & 필터 토글<a name="hw5-2"></a>

```
정적 물체: Wood Tower, 동적 물체: wolf .
my_woodtower_diff.png, my_spider_diff.png 를 데이터 파일에 추가하여.
load_png_to_texture()로 업로드.

 'N' / 'L' 키를 눌러 Scene::set_user_filter()에서 GL_NEAREST / GL_LINEAR 로 필터를 즉시 적용합니다.
기본은, GL_LINEAR 로 필터입니다. 

```

##### 1. 정적 물체: Wood Tower 확인 방법
<img width="1365" alt="image" src="https://github.com/user-attachments/assets/43427b86-f778-4852-9bf6-7d88f863b3c0" />

해당 위치로 카메라 main을 이동하여, wood tower를 바라봅니다. (주의 : 혹시나, 해당 wood tower가 여러 조작을 거친뒤에 사라질 수 있는데, 이땐 재실행 하면 다시 보일 것입니다.)

##### 2.동적 물체 : wolf 확인 방법 
<img width="1572" alt="image" src="https://github.com/user-attachments/assets/75ba8ded-e2e0-4235-9dd6-37a521a3eadb" />

고개를 아래로 숙일 수 있도록, S 키를 통해, 메인 카메라의 고개를 아래로 숙이고, z를 통해, 확대해서, wolf의 머리 부분을 봅니다. 


#### 3. 월드 좌표계 광원<a name="hw5-3"></a>
<img width="1716" alt="image" src="https://github.com/user-attachments/assets/05d42620-be50-4d3a-a728-14bfe4ee6f5f" />

```
Light_Parameters 3개(#0,#3,#4)를 천장 고정 위치에 배치하였습니다.
'3' 키가 입력되면 쉐이더 uniform과 scene.light[].light_on 플래그를 동시에 토글하여 전체 월드 램프를 일괄적으로 on/off 합니다.

```

#### 4. 눈 좌표계 광원<a name="hw5-4"></a>
<img width="1283" alt="image" src="https://github.com/user-attachments/assets/de62a2a4-932f-4a7e-a788-2a70a0df02ef" />

```
0 키를 통해 cctv d를 활성화시킨 후,(좌측 맨 위 두번째 카메라) 해당 카메라의 방향을 조절하면,
CCTV‑D 카메라에 고정된 플래시가 움직이는 것을 확인할 수 있습니다.
카메라 ViewMatrix 를 사용하여 갱신함으로써 이러한 효과가 가능하게 되었습니다.

```

#### 5. 모델 좌표계 광원<a name="hw5-5"></a>
<img width="1521" alt="image" src="https://github.com/user-attachments/assets/cb68a69e-1ba5-4d4f-bf2b-684e80559740" />

```
Tiger 모델 머리 위 헤드라이트를 부착하고, ModelMatrix 변환 후 position/spot_dir 업데이트하여 모델링 좌표계 광원을 구현했습니다.
 '5'키를 통해, 해당 광원을 토글할 수 있습니다.
```

#### 6. 투명 다면체<a name="hw5-6"></a>
<img width="1052" alt="image" src="https://github.com/user-attachments/assets/0984ca9a-aa72-48e9-8bd0-6b8cd8613858" />

```
'6' 키로 g_flag_ico_blend를 토글하고, '+/-' 로 g_ico_alpha 조절할 수 있습니다.
정20면체  블렌딩을 구현했습니다.
```

#### 7. 창의적 쉐이더 효과 2종<a name="hw5-7"></a>
<img width="1713" alt="image" src="https://github.com/user-attachments/assets/ea992e8d-15cf-4f9c-8570-817892779694" />

```
* Dragon Lava Dissolve (키 7)
lava.frag 에서 radial‑UV와 cosine flicker를 결합해 녹아내리는 용암 텍스처
+ 깜빡이는(삼각함수 + clamp(0~1))애니메이션을 구현했습니다.

* Ironman Fresnel‑Neon (키 8)
fresnel.frag 에서 rim lighting +네온 글로우 효과를 생성하고,
rim 강도를 시간에 따라 변화시켜 생동감을 부여합니다.
```


