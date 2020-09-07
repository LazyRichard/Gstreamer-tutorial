# Basic tutorial 2: GStreamer concepts

## Goal

* GStreamer의 element가 무엇인지와 어떻게 생성하는지
* 각각의 element를 어떻게 연결하는지
* element의 행동을 어떤게 커스터마이징 하는지
* 어떻게 bus를 확인하여 GStreamer 메시지로부터 에러나 정보를 꺼내는지

## Result

![tutorial_2](/assets/tutorial_2.gif)

## Walkthrough

element는 GStreamer의 기본 구성 요소이다. source element에서 sink element로 데이터를 보내며 

![Example pipeline](https://gstreamer.freedesktop.org/documentation/tutorials/basic/images/figure-1.png)

## The GStreamer bus

메시지는 `gst_bus_timed_pop_filtered()`를 이용해 동기적으로 꺼낼 수도 있지만 다음 튜토리얼에서 할 예정 인 것처럼 시그널을 이용해 비동기적으로 꺼낼 수도 있다. 어플리케이션은 항상 버스를 주시하면서 에러나 다른 플레이백 관련 이슈들에 대한 알림을 귀기울여야 한다.

# Exercise 1

한 번 source와 sink 사이에 filter를 `vertigotv` 필터를 넣어보라. 먼저 element를 생성하고 pipeline에 추가하고 다른 element와 연결하는 작업이 필요하다.

플랫폼과 사용 가능한 플러그인에 따라서 "negotioation" 에러를 만날 수도 있다. 이 경우에는 `videoconvert`를 filter 다음에 넣어보라.

## Result

![exercise_1](/assets/tutorial_2_exercise.gif)

# Conclusion

* element는 `gst_element_factory_make()`를 이용해 만든다.
* `gst_pipeline_new()`를 이용해 비어있는 파이프라인을 만들 수 있다.
* `gst_bin_add_many()`를 이용해 파이프라인에 element를 여러개 추가 할 수 있다.
* `gst_element_link()`를 이용해 각각의 element를 연결할 수 있다.

# References

* Gstreamer official tutorials - [Basic tutorial 2: GStreamer concepts](https://gstreamer.freedesktop.org/documentation/tutorials/basic/concepts.html?gi-language=c)
