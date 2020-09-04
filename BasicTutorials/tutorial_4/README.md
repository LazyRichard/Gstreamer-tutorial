# Basic tutorial 4: Time management (Partial)

## Goal

* 현재 재생 위치 또는 길이와 같은 pipeline의 정보를 어떻게 조회하는지
* 스트림의 시간상 다른 위치로 어떻게 탐색(Seek) 하는지

## Introduction

`GstQuery`는 element 또는 pad에 대해 정보를 요청하는 방법을 제공한다. 이 예제에서는 pipeline에게 탐색(Seek)이 허용되어 있는지-라이브 스트림과 같은 몇몇 소스에서는 탐색(Seek)이 불가능하기도 하다-를 조회한다. 만약 탐색(Seek)이 가능하다면 10초간 영상을 재생하고, 탐색(Seek)을 이용해 다른 위치에서 영상을 재생한다.

이전 예제에서는 pipeline를 만들고 동작시키면 버스를 통해 `GST_STATE_ERROR`과 `GST_STATE_EOS` 메시지만 기다리도록 설정했다. 이번에는 주기적으로 깨우면서 pipeline의 상태를 조회하여 스트림의 현재 위치를 화면상에 출력한다. 이것은 미디어 재생기가 주기적으로 UI를 업데이트하는 것과 비슷하다.

## Seeking example

![image](/assets/tutorial_4.gif)

## Walkthrough

```c
/* Structure to contain all out information, so we can pass it around */
typedef struct _CustomData
{
    GstElement *playbin;   // Out one and only element
    gboolean playing;      // Are we in the PLAYING state
    gboolean terminate;    // Sould we terminate execution?
    gboolean seek_enabled; // Is seeking enabled for this media?
    gboolean seek_done;    // Have we performed the seek already?
    gint64 duration;       // How long does this media last, in nanoseconds
} CustomData;

/* Forward definition of the message processing function */
static void handle_message(CustomData *, GstMessage *);
```

이번 예제에서는 메시지를 처리하는 코드의 규모가 커져서 `handle_message`로 분리했다.

첫 번째 튜토리얼 처럼 `playbin` element를 갖는 pipeline을 만든다. 하지만 `playbin`이 pipeline 자체 이고 pipeline에는 오직 하나의 element만 있으므로 이번에는 `playbin`을 직접 조작한다. 그리고 URI를 설정하고 pipeline의 상태를 `GST_STATE_PLAYING`으로 변경하는 등의 자잘한 부분은 넘어가겠다.

```c
msg = gst_bus_timed_pop_filtered(bus, 100 * GST_MSECOND,
        GST_MESSAGE_STATE_CHANGED | GST_MESSAGE_ERROR | GST_MESSAGE_EOS | GST_MESSAGE_DURATION);
```

이전에는 `gst_bus_timed_pop_filtered()`에 대해 타임 아웃을 설정하지 않았었다. 이는 메시지가 들어오기 전까지 아무런 리턴을 하지 않는다는 의미이다. 이번에는 타임 아웃을 `100ms`로 설정했다. 만약 `100ms` 동안 아무런 메시지가 들어오지 않으면 함수는 `NULL`을 리턴한다. 이 로직을 활용해 'UI'를 업데이트 하려 한다.

타임아웃은 `GstClockTime`로 정의된다. 하지만 이는 나노초(nanoseconds)로 정의된다. 코드를 더 읽기 쉽게 만들기 위해 `GST_SECOND`, `GST_MSECOND`등의 메크로에 상수 값을 곱해서 표기한다.

```c
/* We got no message, this means the timeout expired */
if (data.playing)
```

만약 pipeline이 `GST_STATE_PLAYING` 상태면 UI를 새로고침 할 차례다. 다른 상태에서는 대부분의 조회(query)는 실패할 것이기 때문에 `GST_STATE_PLAYING`이 아니면 아무 것도 하지 않는다.

UI 업데이트 주기는 대략 `100ms`로 이 때마다 pipeline을 통해 현재 미디어 위치에 대한 정보를 조회하여 화면에 출력한다. 이는 다음 절에서 자세히 설명하겠지만 미디어 위치와 길이에 대한 정보는 자주 사용하는 쿼리라 쉽게 사용할 수 있는 방법을 제공한다.

```c
/* Query the current position of the stream */
if (!gst_element_query_position(data.playbin, GST_FORMAT_TIME, &current))
{
    g_error("Could not query current position.");
}
```

`gst_element_query_position()`는 query 객체에 대한 관리를 대신 해주면서 미디어에 대한 위치를 바로 반환하는 함수이다.

```c
/* If we didn't know it yet, query the stream duration */
if (!GST_CLOCK_TIME_IS_VALID(data.duration))
{
    if (!gst_element_query_duration(data.playbin, GST_FORMAT_TIME, &data.duration))
    {
        g_error("Coult not query current duration.");
    }
}
```

스트림의 길이를 확인하기 위한 헬퍼 함수로 `gst_element_query_duration()`을 사용한다.

```c
/* Print current position and total duration */
g_message("Position %" GST_TIME_FORMAT " / %" GST_TIME_FORMAT "\r",
          GST_TIME_ARGS(current), GST_TIME_ARGS(data.duration));
```

`GST_TIME_FORMAT`과 `GST_TIME_ARGS` 메크로는 GStreamer의 시간을 사용자 친화적으로 표현하는데 도움을 줍니다.

```c
/* If seeking is enabled, we have not done yet, and the time is right, seek */
if (data.seek_enabled && !data.seek_done && current > 10 * GST_SECOND)
{
    g_message("\nReaced 10s, performing seek...");
    gst_element_seek_simple(data.playbin, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT, 30 * GST_SECOND);
    data.seek_done = TRUE;
}
```

탐색(Seek)을 하기 위해서는 그냥 `gst_element_seek_example()` pipeline. 이 함수는 

`GST_SEEK_FLAG_FLASH`:

`GST_SEEK_FLAG_KEY_UNIT`:

`GST_SEEK_FLAG_ACCURATE`:

## Message Pump

`handle_message` 함수는 pipeline의 bus로 들어오는 모든 메시지를 처리한다. `GST_STATE_ERROR`과 `GST_STATE_EOS`를 처리하는 부분은 이전 예제와 동일하므로 넘어가도록 하겠다.

```c
case GST_MESSAGE_DURATION:
    /* The duration has changed, mark the current one as invalid */
    data->duration = GST_CLOCK_TIME_NONE;
    break;
```

이 메시지는 스트림의 길이가 변할 때 bus에 게시된다. 여기서는 단순히 현재 길이값을 유효하지 않은 것으로 표기하여 나중에 다시 조회할 수 있도록 한다.

```c
case GST_MESSAGE_STATE_CHANGED:
    GstState old_state, new_state, pending_state;
    gst_message_parse_state_changed(msg, &old_state, &new_state, &pending_state);
    if (GST_MESSAGE_SRC(msg) == GST_OBJECT(data->playbin))
    {
        g_message("Pipeline state change dfrom %s to %s:", gst_element_state_get_name(old_state), gst_element_state_get_name(new_state));

        /* Remember whether we are in the PLAYING state of not */
        data->playing = (new_state == GST_STATE_PLAYING);
```

탐색(Seek)과 시간 조회는 일반적으로 `GST_STATE_PAUSED`또는 `GST_STATE_PLAYING` 상태에서 유효한 값을 반환한다. 여기서는 `playing` 변수를 활용해 pipeline이 재생중인지를 추적한다. 그리고 `GST_STATE_PLAYING`상태로 처음 진입하면 pipeline이 탐색(Seek)을 허용하는지 여부를 조회한다.

```c
if (data->playing)
{
    /* We just moved to PLAYING. Check if seeking is possible */
    GstQuery *query;
    gint64 start, end;

    query = gst_query_new_seeking(GST_FORMAT_TIME);
    if (gst_element_query(data->playbin, query))
    {
        gst_query_parse_seeking(query, NULL, &data->seek_enabled, &start, &end);
        if (data->seek_enabled)
        {
            g_message("Seeking is ENABLED from %" GST_TIME_FORMAT " to %" GST_TIME_FORMAT, GST_TIME_ARGS(start), GST_TIME_ARGS(end));
        }
        else
        {
            g_message("Seeking is DIABLED for this stream.");
        }
    }
    else
    {
        g_error("Seeking query failed.");
    }

    gst_query_unref(query);
}
```

`gst_query_new_seeking()`은 `GST_FORMAT_TIME` 형태를 갖는 "seeking" 타입의 조회 객체를 생성한다. 

# Conclusion

* `GstQuery`를 이용해 어떻게 pipeline의 정보를 조회하는지
* 현재 위치 정보, 길이와 같은 표준 정보들을 `gst_element_query_position()` 과 `gst_element_query_duration()`을 통해 획득하는 방법
* `gst_element_seek_simple()`를 활용해 임의의 위치를 탐색하는 방법
* 어떤 상태일때 위의 작업들이 가능한지

# References

* Gstreamer official tutorials - [Basic tutorial 4: Time management](https://gstreamer.freedesktop.org/documentation/tutorials/basic/time-management.html?gi-language=c)
