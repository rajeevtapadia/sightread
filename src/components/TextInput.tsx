import clsx from 'clsx'

type TextInputProps = {
  type: string
  onChange: any
  name?: string
  className?: string
  error?: boolean
  placeholder?: string
  autoFocus?: boolean
  min?: number
  max?: number
  value?: string | number
}
export function TextInput(props: TextInputProps) {
  const { onChange, name, className, error, type, placeholder, autoFocus, min, max, value } = props
  return (
    <input
      type={type}
      name={name}
      onChange={onChange}
      className={clsx(
        className,
        'rounded-md p-2 text-base shadow-[inset_0px_1px_4px_rgba(0,0,0,0.25)]',
        error && 'outline outline-red-600',
      )}
      placeholder={placeholder}
      autoFocus={autoFocus}
      min={min}
      max={max}
      value={value}
    />
  )
}
