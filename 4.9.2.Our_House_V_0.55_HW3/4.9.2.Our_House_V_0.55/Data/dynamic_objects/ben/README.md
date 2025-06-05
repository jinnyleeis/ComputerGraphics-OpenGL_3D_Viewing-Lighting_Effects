ben_vntm_00.geom은 학부 과제를 위해 ben_00.obj를 아래의 구조로 변경한 binary 파일이다.

- # of faces
- 각 face(삼각형)의 vertex당 position, normal, texcoord 저장

- # of material Indicies : material을 구분하기 위한 배열 개수
- material Id가 바뀌는 시점에 material Id와 vertex index offset을 저장

- # of materials 
- 각 material 정보 저장 : Ka, Kd, Ks, Ns, Ke
