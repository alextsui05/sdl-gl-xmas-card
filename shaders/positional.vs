attribute vec4 vPosition;
uniform float originX, originY;
uniform float zoom;
varying vec3 color;
void main()
{
   gl_Position = vPosition;
   gl_Position.x = (originX + gl_Position.x) * zoom;
   gl_Position.y = (originY + gl_Position.y) * zoom;
   color = gl_Position.xyz + vec3(0.5);
}
