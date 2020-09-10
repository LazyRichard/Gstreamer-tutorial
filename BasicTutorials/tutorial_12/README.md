# Basic tutorial 12: Streaming

## Goal

스트리밍은 로컬에 데이터를 다운로드하지 않고 Internet으로부터 직접적으로 데이터를 받는 것을 의미한다.

* 어떻게 버퍼링을 활성화 하는지

## Introduction

## Walkthrough

```c
/* Start playing */
ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
if (ret == GST_STATE_CHANGE_FAILURE) {
  g_error("Unable to set the pipeline to the playing state.\n");
  gst_object_unref(pipeline);
  return -1;
} else if (ret == GST_STATE_CHANGE_NO_PREROLL) {
  data.is_live = TRUE;
}
```

...

```c
case GST_MESSAGE_BUFFERING:
  gint percent = 0;

  /* If the stream is live, we do not care about buffering */
  if (data->is_live)
    break;

  gst_message_parse_buffering(msg, &percent);
  g_message("Buffering (%3d%%)", percent);

  /* Wait until buffering is complete before start/resume playing */
  if (percent < 100)
    gst_element_set_state(data->pipeline, GST_STATE_PAUSED);
  else
    gst_element_set_state(data->pipeline, GST_STATE_PLAYING);

  break;
```

만약 라이브 스트림이면 버퍼링 메시지를 무시한다.

`gst_message_parse_buffering()`를 이용하여 현재 버퍼링 레벨을 얻을 수 있다.

버퍼링 레벨이 100% 미만이면 pipeline을 `GST_STATE_PAUSED` 상태로 설정한다. 그렇지 않으면 재생을 위해 `GST_STATE_PLAYING`으로 설정한다.

초기 시작할 때 보면 재생되기 전에 버퍼링 레벨이 100%까지 차오르는 것을 확인할 수 있다. 그러다 네트워크 속도가 느려지거나 응답이 없어서 버퍼가 기아 상태가 되면 버퍼링 레벨이 100% 미만이라는 메시지를 받을 수 있다. 이 때는 pipeline을 멈추고 충분한 버퍼가 찰 때까지 기다려야 한다.

```c
case GST_MESSAGE_CLOCK_LOST:
  /* Get a new clock */
  gst_element_set_state(data->pipeline, GST_STATE_PAUSED);
  gst_element_set_state(data->pipeline, GST_STATE_PLAYING);

  break;
```

2번 째 네트워크 문제는 클럭을 잃어 버리는 것이다. 이 경우 단순히 pipeline을 `GST_STATE_PAUSED`로 변경하고 다시 `GST_STATE_PLAYING`으로 바꾸면 새로운 클럭이 선택된다.

# Conclusion

* 버퍼링 메시지를 처리하는 법
* 클럭을 잃어버리는 문제에 대처하는 법

# References

* Gstreamer official tutorials - [Basic tutorial 12: Streaming](https://gstreamer.freedesktop.org/documentation/tutorials/basic/streaming.html?gi-language=c)
