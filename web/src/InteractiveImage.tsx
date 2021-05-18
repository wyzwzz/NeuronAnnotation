import React, { useEffect, useRef, useState } from "react";
import TrackballControl from "./FirstPersonController";

const EXP = 0.0001;
function hasDiff(prev: number[], next: number[]): boolean {
  for (let i = 0; i < 3; i++) {
    if (Math.abs(prev[i] - next[i]) > EXP) return true;
  }

  return false;
}

const DEBOUNCE = 5;

const Image: React.FC = () => {
  const [src, setSrc] = useState("");
  const [width, setWidth] = useState(1200);
  const [height, setHeight] = useState(900);
  const [recording, setRecording] = useState(false); //标注模式
  const img = useRef<HTMLImageElement>(null);

  useEffect(() => {
    let loop = 0;
    let lastPosition = [1024, 1408, 1286];
    let lastTarget = [0, 0, -1];
    let lastUp = [0, 1, 0];
    let lastZoom = 20.0;
    let first_point=true;
    let lastX=0.0;
    let lastY=0.0;

    const ws = new WebSocket("ws://127.0.0.1:12121/render");
    ws.binaryType = "arraybuffer";
    ws.onopen = () => {
      console.log("连接渲染服务成功");
      if (!img.current) {
        return;
      }

      const control = new TrackballControl(img.current);
      control.position = [...lastPosition] as [number, number, number];
      control.front = [...lastTarget] as [number, number, number];
      control.up = [...lastUp] as [number, number, number];
      control.rotateSpeed = 50.0;
      control.moveSpeed = 1.0;
      control.zoom = lastZoom;
      control.onRecordStart = () => {
        setRecording(true);
      };
      control.onRecord = (x, y, end) => {
        if (end) {
          setRecording(false);
        }

        ws.send(
          JSON.stringify({
            click: {
              x,
              y,
            },
          })
        );
        if(first_point){
          first_point=false;
        }
        else{
          ws.send(
              JSON.stringify({
                AutoPathGen: {
                  point0:[lastY,lastX],
                  point1:[y,x],
                },
              })
          );
        }
        lastX=x;
        lastY=y;
      };
      ws.send(
        JSON.stringify({
          camera: {
            pos: lastPosition,
            front: lastTarget,
            up: lastUp,
            // zoom: lastZoom,
            zoom:lastZoom,
            n:1,
            f:512
          },
        })
      );

      let i = 0;
      const tick = () => {
        control.move();
        if (++i > DEBOUNCE) {
          i = 0;
          const { position, up, front, zoom } = control;
          const newPosition = [...position];
          const newTarget = [...front];
          const newUp = [...up];
          if (
            hasDiff(newPosition, lastPosition) ||
            hasDiff(newTarget, lastTarget) ||
            hasDiff(newUp, lastUp) ||
            Math.abs(lastZoom - zoom) > EXP
          ) {
            lastPosition = [...newPosition];
            lastTarget = [...newTarget];
            lastUp = [...newUp];
            lastZoom = zoom;
            ws.send(
              JSON.stringify({
                camera: {
                  pos: newPosition,
                  front: newTarget,
                  up: newUp,
                  zoom:zoom,
                  n:1,
                  f:512
                },
              })
            );
          }
        }
        window.requestAnimationFrame(tick);
      };
      loop = window.requestAnimationFrame(tick);
    };
    ws.onmessage = (msg) => {
      const { data } = msg;
      if (typeof msg.data === "object") {
        const bytes = new Uint8Array(msg.data);
        const blob = new Blob([bytes.buffer], { type: "image/jpeg" });
        const url = URL.createObjectURL(blob);
        setSrc(url);
        return;
      }

      try {
        const obj = JSON.parse(data);
        console.log(obj.error);
      } catch {
        console.log(data);
      }
    };
    ws.onerror = () => {
      console.error("连接渲染服务出现错误");
    };

    return () => {
      ws.close();
      window.cancelAnimationFrame(loop);
    };
  }, []);

  return (
    <div>
      <img
        className="interactive-img"
        src={src}
        width={width}
        height={height}
        ref={img}
        alt=""
        style={{
          width: `${width}px`,
          height: `${height}px`,
          opacity: src ? 1 : 0,
        }}
      />

      <div>{recording ? "已进入标注模式，右键添加标注点" : "按R进入标注模式，F键退出标注模式"}</div>
    </div>
  );
};

export default Image;
