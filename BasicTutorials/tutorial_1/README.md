# Basic tutorial 1: Hello World!

## Hello world

## Results
![tutorial_1](/assets/tutorial_1.gif)

## Walkthrough

`gst_init`은 GStreamer의 실행 환경을 구성해주는 함수로 어플리케이션이 실행되기 전에 항상 먼저 실행되여야 합니다.

```c
/* Initialize GStreamer */
gst_init (&argc, &aragv);
```

* 모든 내부 구조체 초기화
* 어떤 플러그인이 사용가능한지 확인
* GStreamer 위해 정의된 커맨드라인 옵션 실행


```c
gchar* uri = "playbin uri=https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm";

...

/* Build the pipeline */
pipeline = (GstPipeline*)gst_parse_launch(uri, NULL);
```

### gst_parse_launch

GStreamer는 멀티미디어의 흐름을 핸들링하기 위한 프레임워크이다. 미디어는 'source'라는 element에서 'sink' element로 흐르면서 중간에 여러 작업들을 수행하는 element를 거치게 된다. 그리고 이렇게 연결된 element들의 묶음을 pipeline이라고 부른다.

일반적으로 GStreamer를 통해 pipeline를 구축할 때는 여러 다양한 element들을 손수 조합하게 되는데, 그런 복잡한 기능 없이 아주 간단하게 사용하고 싶은 경우에는 `gst_parse_launch()` 함수를 활용할 수 있다. 이 함수는 텍스트 형태로 pipeline을 기술하면 이를 pipeline 객체로 만들어준다.

### playbin

`gst_parse_launch()`함수를 호출하면 `playbin`이라는 element를 만들어 준다. 이 playbin은 특별한 element로 source와 sink역할을 수행하며 pipeline 전체를 나타낸다. 내부적으로 미디어 재생에 필요한 모든 element에 대한 생성과 연결을 수행하니 너무 걱정할 필요는 없다.

수동으로 구성된 pipeline처럼 세분화된 설정을 제공하지는 않지만 이 튜토리얼을 포함한 여러 광범위한 어플리케이션에서는 충분히 만족할만한 수준의 설정을 할 수 있다.

```c
/* Start playing */
gst_element_set_state(pipeline, GST_STATE_PLAYING);
```

여기서 `gst_element_set_state()`를 통해 `pipeline` 객체의 상태를 PLAYING으로 변경하여 미디어 재생을 시작한다.

```c
bus = gst_element_get_bus(pipeline);

/* Wait until error or EOS */
msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, GST_MESSAGE_ERROR | GST_MESSAGE_EOS);
```

이 라인들은 에러가 발생하거나 스트림의 끝이라는 메시지를 받을 때까지 기다린다. 먼저 `gst_element_get_bus()`를 통해 파이프라인의 버스를 가져오고 `gst_bus_timed_pop_filterd()`를 통해 에러나 EOS 메시지가 버스에 들어올 때까지 블럭한다. 버스에 대해서는 Tutorial2를 참고하시라.

# Conclusion

* GStreamer를 초기화 하려면 `gst_init()`함수를 **꼭** 사용해야 한다.
* 텍스트 형태로 간단하게 파이프라인을 구성하려면 `gst_parse_launch()`을 호출하면 된다.
* GStremaer의 파이프라인의 상태를 관리하려면 `gst_element_set_state()`를 사용하면 된다.
* 함수를 사용하기 전에 cleanup을 해야 하는지 확인해서 프로그램 종료 전에 또는 필요 없는 객체들은 cleanup을 해주어야 한다.


# References
* Gstreamer official tutorials - [Basic tutorial 1: Hello world!](https://gstreamer.freedesktop.org/documentation/tutorials/basic/hello-world.html?gi-language=c)
