<program>
  <attribute name="position" unit="0"></attribute>
  <attribute name="normal" unit="1"></attribute>
  <attribute name="color" unit="2"></attribute>
  <attribute name="tracks" unit="3"></attribute>
  <attribute name="ao" unit="4"></attribute>

  <shader type="vertex">
  <![CDATA[
  in vec3 position;
  in vec3 normal;
  in vec2 color;
  in vec2 tracks;
  in vec2 ao;
  out vec3 pnormal;
  out vec2 pcolor;
  out vec2 ptracks;
  out vec2 pao;
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
    ptracks = tracks;
    pao = ao;
  }
  ]]>
  </shader>

  <shader type="pixel">
  <![CDATA[
  in vec3 pnormal;
  in vec2 pcolor;
  in vec2 ptracks;
  in vec2 pao;
  in vec4 shadowCoord;
  uniform sampler2D texture0;
  uniform sampler2D texture1;
  uniform sampler2D texture2;
  uniform sampler2D depth;
  uniform mat4 modelView;
  uniform vec3 light_dir;

  const vec3 light_diffuse = vec3(0.8, 0.8, 0.8);

  void main() {
    vec4 L = vec4(normalize((modelView * vec4(light_dir, 0)).xyz), 0);
    vec4 N = vec4(normalize(pnormal), 0);
    vec4 diffuse = vec4(max(dot(N, L), 0.0) * light_diffuse, 0);
    vec4 base = texture2D(texture0, pcolor);
    vec4 track = texture2D(texture1, ptracks);
    vec4 ao = texture2D(texture2, pao) + vec4(0.2, 0.2, 0.2, 1);
    /*vec4 ambient = vec4(0.1, 0.1, 0.1, 1);*/

		vec4 shadowCoordinateWdivide = shadowCoord / shadowCoord.w;
		shadowCoordinateWdivide.z += 0.0005;
		float distanceFromLight = texture2D(depth, shadowCoordinateWdivide.st).z;
	 	float shadow = 1.0;
	 	if (shadowCoord.w > 0.0)
	 	  shadow = distanceFromLight < shadowCoordinateWdivide.z ? 0.5 : 1.0;
    if (shadowCoord.x > 1.0 || shadowCoord.y > 1.0 || shadowCoord.x < 0.0 || shadowCoord.y < 0.0)
      shadow = 1.0;
    gl_FragColor = mix(base, track, track.a) * (diffuse + ao - 0.5) * shadow;
    gl_FragColor.a = 1;
  }
  ]]>
  </shader>
</program>
