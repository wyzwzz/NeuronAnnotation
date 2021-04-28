import Vector3 from "./math/Vector3.js";
import Vector2 from "./math/Vector2.js";
import Quaternion from "./math/Quaternion.js";

enum MouseState {
  NONE,
  ROTATE,
  PAN,
  ZOOM,
}

enum MouseButtons {
  LEFT = 0,
  MIDDLE = 1,
  RIGHT = 2,
}

class FirstPersonConroller {
  el: HTMLElement;
  enabled: boolean;
  viewport: {
    left: number;
    top: number;
    width: number;
    height: number;
  };
  state: MouseState;
  position: [number, number, number];
  front: [number, number, number];
  up: [number, number, number];
  zoom: number;
  lastMouse: [number, number];
  currMouse: [number, number];
  yaw:number;
  pitch:number;
  rotateSpeed: number;
  zoomSpeed: number;
  moveSpeed: number;
  movement: [number, number, number];
  world_up:[number,number,number];

  recording: boolean;
  onRecord?: (x: number, y: number, end: boolean) => void;
  onRecordStart?: () => void;

  constructor(el: HTMLElement) {
    this.el = el;
    this.enabled = true;
    this.viewport = {
      left: 0,
      top: 0,
      width: 0,
      height: 0,
    };
    this.state = MouseState.NONE;
    this.position = [0, 0, 100];
    this.front = [0, 0, -1];
    this.up = [0, 1, 0];
    this.zoom = 1.0;
    this.lastMouse = [0, 0];
    this.currMouse = [0, 0];
    this.rotateSpeed = 10.0;
    this.moveSpeed = 1.0;
    this.zoomSpeed = 1.0;
    this.movement = [0.0, 0.0, 0.0];
    this.recording = false;
    this.yaw=-90.0;
    this.pitch=0.0;
    this.world_up=[0.0,1.0,0.0];
    this.addEventListener();
    this.onResize();
  }

  onResize = () => {
    const box = this.el.getBoundingClientRect();
    // adjustments come from similar code in the jquery offset() function
    const d = this.el.ownerDocument.documentElement;
    this.viewport.left = box.left + window.pageXOffset - d.clientLeft;
    this.viewport.top = box.top + window.pageYOffset - d.clientTop;
    this.viewport.width = box.width;
    this.viewport.height = box.height;
  };

  getMouseOnScreen(pageX: number, pageY: number) {
    return new Vector2(
      (pageX - this.viewport.left) / this.viewport.width,
      (pageY - this.viewport.top) / this.viewport.height
    );
  }

  getMouseOnCircle(pageX: number, pageY: number) {
    const { left, top, width, height } = this.viewport;
    return new Vector2(
      (pageX - width * 0.5 - left) / (width * 0.5),
      (height + 2 * (top - pageY)) / width // screen.width intentional
    );
  }

  addEventListener() {
    this.el.addEventListener("contextmenu", this.onContextmenu, false);
    this.el.addEventListener("mousedown", this.onMousedown, false);
    this.el.addEventListener("wheel", this.onMousewheel, false);

    // this.el.addEventListener("touchstart", this.touchstart, false);
    // this.el.addEventListener("touchend", this.touchend, false);
    // this.el.addEventListener("touchmove", this.touchmove, false);

    window.addEventListener("keydown", this.onKeydown, false);
    window.addEventListener("keyup", this.onKeyup, false);
  }

  removeEventListener() {
    this.el.removeEventListener("contextmenu", this.onContextmenu, false);
    this.el.removeEventListener("mousedown", this.onMousedown, false);
    this.el.removeEventListener("wheel", this.onMousewheel, false);

    // this.el.removeEventListener("touchstart", this.touchstart, false);
    // this.el.removeEventListener("touchend", this.touchend, false);
    // this.el.removeEventListener("touchmove", this.touchmove, false);

    document.removeEventListener("mousemove", this.onMousemove, false);
    document.removeEventListener("mouseup", this.onMouseup, false);

    window.removeEventListener("keydown", this.onKeydown, false);
    window.removeEventListener("keyup", this.onKeyup, false);
  }

  onContextmenu = (e: Event) => {
    if (!this.enabled) {
      return;
    }

    e.preventDefault();
    e.stopPropagation();
  };

  onMousedown = (event: MouseEvent) => {
    if (!this.enabled) return;

    event.preventDefault();
    event.stopPropagation();

    if (this.state === MouseState.NONE) {
      switch (event.button) {
        case MouseButtons.LEFT:
          this.state = MouseState.ROTATE;
          break;
        case MouseButtons.MIDDLE:
          this.state = MouseState.ZOOM;
          break;
        case MouseButtons.RIGHT:
          this.state = MouseState.PAN;
          break;
        default:
          this.state = MouseState.NONE;
      }
    }

    if (this.state === MouseState.ROTATE) {
      const mouse = this.getMouseOnScreen(event.pageX, event.pageY);
      this.lastMouse = [mouse.x, mouse.y];
    } else if (this.state === MouseState.PAN) {
      const mouse = this.getMouseOnScreen(event.pageX, event.pageY);
      this.lastMouse = [mouse.x, mouse.y];
    }

    this.el.style.cursor = "grabbing";
    document.addEventListener("mousemove", this.onMousemove, false);
    document.addEventListener("mouseup", this.onMouseup, false);
  };

  onMouseup = (event: MouseEvent) => {
    if (!this.enabled) {
      return;
    }

    event.preventDefault();
    event.stopPropagation();

    this.state = MouseState.NONE;
    this.el.style.cursor = "grab";

    document.removeEventListener("mousemove", this.onMousemove);
    document.removeEventListener("mouseup", this.onMouseup);
    if (event.button === 2 && this.recording && this.onRecord) {
      this.onRecord(
        Math.round(event.pageX - this.viewport.left),
        Math.round(event.pageY - this.viewport.top),
        false
      );
    }
  };

  onMousemove = (event: MouseEvent) => {
    if (!this.enabled) {
      return;
    }

    event.preventDefault();
    event.stopPropagation();

    if (this.state === MouseState.ROTATE) {
      const mouse = this.getMouseOnScreen(event.pageX, event.pageY);
      this.currMouse = [mouse.x, mouse.y];
      this.updateCamera();
      // this.rotateCamera();
    } else if (this.state === MouseState.PAN) {
      const mouse = this.getMouseOnScreen(event.pageX, event.pageY);
      this.currMouse = [mouse.x, mouse.y];
      // this.panCamera();
    }
  };

  updateCamera=()=>{
      this.yaw+=(this.lastMouse[0]-this.currMouse[0]);
      this.pitch-=(this.lastMouse[1]-this.currMouse[1]);
      if(this.pitch>60.0){
        this.pitch=60.0;
      }
      else if(this.pitch<-60.0){
        this.pitch=-60.0;
      }
      const x=Math.cos(this.yaw*Math.PI/180.0)*Math.cos(this.pitch*Math.PI/180.0);
      const y=Math.sin(this.pitch*Math.PI/180.0);
      const z=Math.sin(this.yaw*Math.PI/180.0)*Math.cos(this.pitch*Math.PI/180.0);
      let front=new Vector3(x,y,z);
      front.normalize();
      const world_up=new Vector3(this.world_up[0],this.world_up[1],this.world_up[2]);
      const right=new Vector3();right.crossVectors(front,world_up).normalize();
      const up=new Vector3();up.crossVectors(right,front).normalize();
      this.front=[front.x,front.y,front.z];
      this.up=[up.x,up.y,up.z];
    
  };

  onMousewheel = (event: WheelEvent) => {
    if (this.enabled === false) return;

    event.preventDefault();
    event.stopPropagation();

    let zoom = 1.0;
    switch (event.deltaMode) {
      case 2:
        // Zoom in pages
        zoom = event.deltaY * 0.025;
        break;

      case 1:
        // Zoom in lines
        zoom = event.deltaY * 0.01;
        break;

      default:
        // undefined, 0, assume pixels
        zoom = event.deltaY * 0.00025;
        break;
    }

    this.zoomCamera(zoom);
  };

  onKeydown = (e: KeyboardEvent) => {
    if (e.key === "a") {
      this.movement[0] = -1.0;
    } else if (e.key === "d") {
      this.movement[0] = 1.0;
    } else if (e.key === "w") {
      this.movement[2] = 1.0;
    } else if (e.key === "s") {
      this.movement[2] = -1.0;
    } else if (e.key === "q") {
      this.movement[1] = 1.0;
    } else if (e.key === "e") {
      this.movement[1] = -1.0;
    } else if (e.key === "r") {
      this.recording = true;
      if (this.onRecordStart) {
        this.onRecordStart();
      }
    }
  };

  onKeyup = (e: KeyboardEvent) => {
    if (e.key === "a") {
      this.movement[0] = 0.0;
    } else if (e.key === "d") {
      this.movement[0] = 0.0;
    } else if (e.key === "w") {
      this.movement[2] = 0.0;
    } else if (e.key === "s") {
      this.movement[2] = 0.0;
    } else if (e.key === "q") {
      this.movement[1] = 0.0;
    } else if (e.key === "e") {
      this.movement[1] = 0.0;
    } else if (e.key === "f") {
      this.recording = false;
      if (this.onRecord) {
        this.onRecord(0, 0, true);
      }
    }
  };

  // rotateCamera = () => {
  //   const axis = new Vector3();
  //   const quaternion = new Quaternion();
  //   const eyeDirection = new Vector3();
  //   const cameraUpDirection = new Vector3();
  //   const cameraSidewaysDirection = new Vector3();
  //   const moveDirection = new Vector3(
  //     this.currMouse[0] - this.lastMouse[0],
  //     this.currMouse[1] - this.lastMouse[1],
  //     0
  //   );
  //   let angle = moveDirection.length();

  //   const eye = new Vector3(
  //     this.position[0] - this.target[0],
  //     this.position[1] - this.target[1],
  //     this.position[2] - this.target[2]
  //   );
  //   const position = new Vector3(
  //     this.position[0],
  //     this.position[1],
  //     this.position[2]
  //   );
  //   const target = new Vector3(this.target[0], this.target[1], this.target[2]);
  //   const up = new Vector3(this.up[0], this.up[1], this.up[2]);

  //   if (angle) {
  //     eye.copy(position).sub(target);

  //     eyeDirection.copy(eye).normalize();
  //     cameraUpDirection.copy(up).normalize();
  //     cameraSidewaysDirection
  //       .crossVectors(cameraUpDirection, eyeDirection)
  //       .normalize();

  //     cameraUpDirection.setLength(this.currMouse[1] - this.lastMouse[1]);
  //     cameraSidewaysDirection.setLength(this.currMouse[0] - this.lastMouse[0]);

  //     moveDirection.copy(cameraUpDirection.add(cameraSidewaysDirection));

  //     eye.addVectors(eye, moveDirection.multiplyScalar(this.rotateSpeed));

  //     target.copy(position).sub(eye);
  //     this.target = [target.x, target.y, target.z];
  //   }

  //   this.lastMouse = [...this.currMouse];
  // };

  zoomCamera = (zoom: number) => {
    let factor;

    factor = 1.0 + zoom * this.zoomSpeed;

    this.zoom /= factor;
    this.zoom = Math.max(0.00001, this.zoom);
    this.zoom = Math.min(this.zoom,45.0);
    return;
  };

  move = () => {

    const position = new Vector3(
      this.position[0],
      this.position[1],
      this.position[2]
    );
    const front = new Vector3(this.front[0], this.front[1], this.front[2]);
    const up = new Vector3(this.up[0], this.up[1], this.up[2]);
    console.log(front);
    // eye.copy(position).sub(target);

    const eyeDirection = new Vector3();
    const cameraUpDirection = new Vector3();
    const cameraSidewaysDirection = new Vector3();
    eyeDirection.copy(front).normalize();
    cameraUpDirection.copy(up).normalize();
    cameraSidewaysDirection
      .crossVectors(eyeDirection,cameraUpDirection)
      .normalize();

    position.addVectors(
      position,
      eyeDirection.multiplyScalar(this.movement[2] * this.moveSpeed)
    );
    position.addVectors(
      position,
      cameraUpDirection.multiplyScalar(this.movement[1] * this.moveSpeed)
    );
    position.addVectors(
      position,
      cameraSidewaysDirection.multiplyScalar(this.movement[0] * this.moveSpeed)
    );

    this.position = [position.x, position.y, position.z];
    // console.log(eyeDirection);
    // console.log(this.movement[0] * this.moveSpeed)
    // console.log(this.movement[1] * this.moveSpeed)
    // console.log(this.movement[2] * this.moveSpeed)
  };

}

export default FirstPersonConroller;
