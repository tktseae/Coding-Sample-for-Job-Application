import torch
from torchinfo import summary

# Training function
def train(model, device, criterion, optimizer, train_loader, valid_loader, epochs):
  train_acc, val_acc, train_loss, val_loss = [], [], [], []

  for epoch in range(1, epochs+1):
    # Training
    model.train()
    correct, total = 0, 0
    loss = 0.0
    for idx, sample in enumerate(train_loader):
      input_ids = sample["input_ids"].to(device, dtype = torch.long)
      attention_mask = sample["attention_mask"].to(device, dtype = torch.long)
      token_type_ids = sample["token_type_ids"].to(device, dtype = torch.long)
      label = sample["label"].to(device)

      # if idx == 0: return [], [], [], []

      softmax_output = model(input_ids, attention_mask, token_type_ids)

      loss = criterion(softmax_output, label)
      optimizer.zero_grad()
      loss.backward()
      optimizer.step()

      # Calculate accuracy
      pred_label = torch.argmax(softmax_output, dim=1)

      label = label.data.cpu()
      pred_label = pred_label.data.cpu()
      
      correct += (pred_label == label).sum().item()
      total += label.size(0)

      if idx % 100 == 0:
        # print(f"epoch: {epoch}, loss: {loss:.3f}, train_acc = {correct} / {total} = {(correct/total):.4f}")
        print(f"epoch: {epoch}, loss: {loss:.3f}, train_acc: {(correct/total):.4f}")

    train_acc.append(correct/total)
    train_loss.append(loss.data.cpu().item())

    # Validation
    model.eval()
    valid_correct, valid_total = 0, 0
    loss = 0.0
    with torch.no_grad():
      for idx, sample in enumerate(valid_loader):
        input_ids = sample["input_ids"].to(device, dtype = torch.long)
        attention_mask = sample["attention_mask"].to(device, dtype = torch.long)
        token_type_ids = sample["token_type_ids"].to(device, dtype = torch.long)
        label = sample["label"].to(device)

        softmax_output = model(input_ids, attention_mask, token_type_ids)
        loss = criterion(softmax_output, label)
        pred_label = torch.argmax(softmax_output, dim=1)

        label = label.data.cpu()
        pred_label = pred_label.data.cpu()

        valid_correct += (pred_label == label).sum().item()
        valid_total += label.size(0)
    val_acc.append(valid_correct/valid_total)
    val_loss.append(loss.data.cpu().item())
    print(f"=== epoch: {epoch}, val_loss: {loss:.3f}, valid_acc: {(valid_correct/valid_total):.4f} ===")

  return train_acc, val_acc, train_loss, val_loss
