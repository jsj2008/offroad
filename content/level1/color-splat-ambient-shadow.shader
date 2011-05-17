<program>
  <attribute name="position" unit="0"></attribute>
  <attribute name="normal" unit="1"></attribute>
  <attribute name="color" unit="2"></attribute>

  <shader type="vertex">
  <![CDATA[
  in vec3 position;
  in vec3 normal;
  in vec2 color;
  out vec3 pnormal;
  out vec2 pcolor;
  out vec4 shadowCoord;
  uniform mat4 modelView;
  uniform mat4 proj;
  uniform mat4 bias;
  uniform mat4 shadowProj;
  uniform mat4 shadowModelView;
  uniform mat4 model;
 
  void main() {
    gl_Position = proj * modelView * vec4(position, 1);
    shadowCoord = bias * shadowProj * shadowModelView * model * vec4(position, 1);
    pnormal = (modelView * vec4(normal, 0)).xyz;
    pcolor = color;
  }
  ]]>
  </shader>

  <shader type="pixel">
  <![CDATA[
  in vec3 pnormal;
  in vec2 pcolor;
  in vec4 shadowCoord;
  uniform sampler2D texture0;
  uniform sampler2D depth;
  uniform mat4 modelView;
  uniform vec3 light_dir;

  const vec3 light_diffuse = vec3(0.8, 0.8, 0.8);

  void main() {
    vec4 L = vec4(normalize((modelView * vec4(light_dir, 0)).xyz), 0);
    vec4 N = vec4(normalize(pnormal), 0);
    vec4 diffuse = vec4(max(dot(N, L), 0.0) * light_diffuse, 0);
    vec4 frag_color = texture2D(texture0, pcolor);

		vec4 shadowCoordinateWdivide = shadowCoord / shadowCoord.w;
		shadowCoordinateWdivide.z += 0.0005;
		float distanceFromLight = texture2D(depth, shadowCoordinateWdivide.st).z;
	 	float shadow = 1.0;
	 	if (shadowCoord.w > 0.0)
	 	  shadow = distanceFromLight < shadowCoordinateWdivide.z ? 0.5 : 1.0;
    gl_FragColor = frag_color * diffuse * shadow;
  }
  ]]>
  </shader>
</program>
