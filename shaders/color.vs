attribute vec3 vPosition;
uniform vec3 uColor;
varying vec3 color;

void main() {
    gl_Position = vec4(vPosition, 1);

    color = uColor;
}
