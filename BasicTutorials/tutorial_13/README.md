# Basic tutorial 13: Playback speed

## Goal

빨리감기, 되감기와 슬로우 모션은 통상적으로 trick 모드라고 불리는데 모두 플레이 레이트를 수정한다는 공통점이 있다.

* 플레이 레이트를 수정하여 어떻게 빠르게 또는 느리게, 앞으로 또는 뒤로 재생하는 지
* 영상을 프레임단위로 움직이는 방법

## Introduction

빨리 감기는 미디어를 일반 속도보다 빠르게 재생하는 것을 말한다. 슬로우 모션은 느리게 재생하는 것을 말한다. 되감기는 비슷하지만 방향이 미디어의 끝 부분 부터 시작해 시작부분으로 가는 역방향이다.

이러한 모든 기법은 플레이 레이트를 조정을 통해 이뤄진다. 1.0이면 정상 속도를, 1.0보다 크면 빠른 모드, 1.0보다 작으면 느린 모드, 양수면 정방향 재생 음수면 역방향 재생이다.

GStreamer에서는 플레이 레이트를 변경할 수 있도록 **Step Event**와 **Seek Event** 총 2가지 방법을 제공한다. **Step Event**는 플레이 레이트를 양수값으로 변경하는 것 이외에도 일정 부분 미디어를 건너뛸 수 있다. **Seek Event**는 추가적으로 양의 방향 또는 음의 방향 모두 스트림의 어느 위치로도 건너뛸 수 있다.

**Step Event**는 요구하는 파라미터의 수가 적어 플레이 레이트를 변경하는데 조금 더 편하지만 몇 가지 단점이 있어 여기서는 **Seek Event**를 사용한다. **Step Event**는 pipeline의 끝단인 sink만 적용할 수 있다. 따라서 나머지 pipeline이 다른 속도로 동작하는 것이 가능해야 한다. 반면에 **Seek Event**는 pipeline을 통해 모든 element가 반응할 수 있도록 한다. **Step Event**는 재생하는 방향을 바꿀 수는 없지만 반응이 매우 빠르다는 장점이 있다.

...



# References

* Gstreamer official tutorials - [Basic tutorial 13: Playback speed](https://gstreamer.freedesktop.org/documentation/tutorials/basic/playback-speed.html?gi-language=c)
