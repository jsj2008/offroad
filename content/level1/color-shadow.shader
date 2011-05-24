<program>
  <attribute name="position" unit="0"></attribute>
  <attribute name="normal" unit="1"></attribute>
  <attribute name="color" unit="2"></attribute>

  <shader type="vertex">
  in vec3 position;
  in vec3 normal;
  in vec2 color;
  out vec3 pnormal;
  out vec2 pcolor;
  out vec4 shadowCoord;
  uniform mat4 modelView;
  uniform mat4 model;
  uniform mat4 proj;
  uniform mat4 bias;
  uniform mat4 shadowProj;
  uniform mat4 shadowModelView;
 
  void main() {
    gl_Position = proj * modelView * vec4(position, 1);
    shadowCoord = bias * shadowProj * shadowModelView * model * vec4(position, 1);
    pnormal = (modelView * model * vec4(normal, 0)).xyz;
    pcolor = color;
  }
  </shader>

  <shader type="pixel">
  <![CDATA[
  in vec3 pnormal;
  in vec2 pcolor;
  in vec4 shadowCoord;
  uniform sampler2D texture0;
  uniform sampler2D depth;

  void main() {
		vec4 shadowCoordinateWdivide = shadowCoord / shadowCoord.w;
		shadowCoordinateWdivide.z += 0.0005;
		float distanceFromLight = texture2D(depth, shadowCoordinateWdivide.st).z;
	 	float shadow = 1.0;
	 	if (shadowCoord.w > 0.0)
	 	  shadow = distanceFromLight < shadowCoordinateWdivide.z ? 0.5 : 1.0;

    vec4 base = texture2D(texture0, pcolor);
    gl_FragColor = base * shadow;
    gl_FragColor.a = 1;
  }
  ]]>
  </shader>
</program>
