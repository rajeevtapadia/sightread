import * as RadixToast from '@radix-ui/react-toast'

interface props {
  open: boolean
  title: string
  description?: string
  removeToast: () => void
}

export default function Toast({ open, description, title, removeToast }: props) {
  return (
    <RadixToast.Root
      open={open}
      onOpenChange={(open) => {
        if (!open) {
          removeToast()
        }
      }}
      className="flex flex-col gap-1 rounded-lg bg-[#292929] p-4 text-white shadow-lg"
    >
      <RadixToast.Title className="font-semibold">{title}</RadixToast.Title>
      <RadixToast.Description className="text-xs text-gray-300">
        {description}
      </RadixToast.Description>
    </RadixToast.Root>
  )
}
