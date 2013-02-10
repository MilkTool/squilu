function puts(s) {
	fd.write(s);
}

function putsnl(s) {
	fd.write(s);
	fd.write("\n");
}

function preprocess(file_name){
	local fd = file(file_name, "r");
	local code = fd.read(fd.len());
	fd.close();
	
	local function escape_re(str){
		local new_str = str.gsub("[-.$%%[%]^]", "%%%1")
		return new_str
	}
	local code_generation_begin = "// generated-code:begin";
	local code_generation_end = "// generated-code:end";

	local code_generation_begin_escaped = escape_re(code_generation_begin);
	local code_generation_end_escaped = escape_re(code_generation_end);

	//print(code_generation_begin, code_generation_begin_escaped);

	local new_code = code.gsub(code_generation_begin_escaped + ".-" + code_generation_end_escaped, "");

	new_code = new_code.gsub("(//@(.-)\n)", function(m, m2) {
			return format("%s%s]====])\n%s;\nputsnl([====[\n%s", m, code_generation_begin, m2, code_generation_end)
		});


	new_code = new_code.gsub("(/%*SquiLu(.-)SquiLu%*/)", function(m, m2) {
			return format("%s]====])\n%s\nputsnl([====[", m, m2)
		});

	local buffer = blob();
	buffer.write("putsnl([====[");
	buffer.write(new_code);
	buffer.write("]====])");
	local sqcode = buffer.tostring();
	
	local code_func = compilestring(sqcode, "sqcode-preprocessed");

	::fd <- file(file_name + ".cpp", "w");
	code_func();
	::fd.close();
}

if(vargv.len() > 0){
	preprocess(vargv[0]);
}
