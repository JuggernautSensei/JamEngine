# Copilot Instructions

## General Guidelines
- When modifying code in this repo, add a trailing `// AI` comment to the exact lines changed, and do not run builds unless explicitly asked.
- Confirm that SRV/RTV conflicts within the same pass of RenderGraph are properly prevented by the current code (e.g., Validate_).
- Prefer building a single project (project build) by default instead of building the whole solution.
- 사용자는 RenderGraph/Compile의 상태 불일치 시 방어적으로 복구하기보다 `at()` 등으로 즉시 크래시(죽는 동작)가 더 안전하다고 선호함.

## Language
- Always respond in Korean.